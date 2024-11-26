#include "ftp_worker.h"

static std::string fixSlashesInURL(const std::string& url) {
    std::string fixed_url = url;
    size_t pos = 0;

    // Заменяем все вхождения '%2F' на '/'
    while ((pos = fixed_url.find("%2F", pos)) != std::string::npos) {
        fixed_url.replace(pos, 3, "/");
        pos += 1; // Сдвигаем позицию, чтобы не зациклиться
    }

    // Заменяем все двойные слэши "//" на один "/"
    while ((pos = fixed_url.find("//", pos)) != std::string::npos) {
        fixed_url.replace(pos, 2, "/");
        pos += 1; // Сдвигаем позицию, чтобы не зациклиться
    }

    return fixed_url;
}
// Обратный вызов для чтения данных из файла
 size_t FtpWorker::read_callback(void* ptr, size_t size, size_t nmemb, void* stream) {
	auto* file = static_cast<std::ifstream*>(stream);
	size_t totalSize = size * nmemb;
	file->read(static_cast<char*>(ptr), totalSize);
	return file->gcount(); // Возвращает количество прочитанных байт
}
 size_t FtpWorker::write_callback_list(char* ptr, size_t size, size_t nmemb, void* list)
{
    size_t realsize = size * nmemb;
    std::string* string_list = static_cast<std::string*>(list);
    string_list->append(ptr, realsize);
    return realsize;
}
  size_t FtpWorker::write_callback(void* ptr, size_t size, size_t nmemb, void* stream) {
	 std::ofstream* file = static_cast<std::ofstream*>(stream);
	 size_t totalSize = size * nmemb;
	 file->write(static_cast<char*>(ptr), totalSize);
	 return totalSize;
 }
long FtpWorker::file_is_coming(struct curl_fileinfo* finfo, struct callback_data* data) {
    callback_data::Info info;
    info.type = finfo->filetype;
    info.name = finfo->filename;
    data->queue_info.push(info);
    return CURL_CHUNK_BGN_FUNC_SKIP;
}

bool FtpWorker::recursiv_list_FTP(CURL* curl, CURLcode& res, const std::string& base_url, std::string subfolder, const std::function<void(const GroupTask&)>& func, const TaskParams& dir)
{
    callback_data current;

    std::string temp_subfolder = fixSlashesInURL(curl_easy_escape(curl, subfolder.data(), subfolder.size()));
    std::string temp_url = base_url + temp_subfolder + "*";
    curl_easy_setopt(curl, CURLOPT_URL, temp_url.c_str());
    curl_easy_setopt(curl, CURLOPT_CHUNK_DATA, &current);
    res = curl_easy_perform(curl);
    if (res == CURLE_REMOTE_FILE_NOT_FOUND)
    {
        return false;
    }
    if (res != CURLE_OK) {
        spdlog::error("Curl request failed: {}", curl_easy_strerror(res));
        return false;
    }
    while (!current.queue_info.empty()) {
        const auto& info = current.queue_info.front();
        if (info.type == CURLFILETYPE_FILE)
        {
            fs::path unesape_file = fs::u8path(subfolder + info.name);
            fs::path relative_path = fs::relative(unesape_file, dir.from.path);
            GroupTask group_task;
            if (dir.to.size() == 1)
            {
                group_task.set_file_mode(FileMode::ONLY_MOVE);
                group_task.push_task(dir.from, dir.to.back(), relative_path, unesape_file);
                current.queue_info.pop();
                continue;

            }
            else if (dir.file_mode == FileMode::BALANCER)
            {
                for (const auto& path_to : dir.to) {
                    group_task.set_file_mode(FileMode::BALANCER);
                    group_task.push_task(dir.from, dir.to.back(), relative_path, unesape_file);
                }
            }
            else {
                for (const auto& path_to : dir.to) {
                    group_task.set_file_mode(FileMode::COPY);
                    group_task.push_task(dir.from, path_to, relative_path, unesape_file);
                }
                // copyNode.try_put(group_task);
            }
            //files.push(fmt::format("{}/{}", base_url, current.queue_info.back().name));
        }
        else if (info.type == CURLFILETYPE_DIRECTORY)
        {
            if (dir.recursiv)
                recursiv_list_FTP(curl, res, base_url, subfolder + current.queue_info.front().name + "/", func,dir);

        }
        current.queue_info.pop();
    }
    return true;
}
bool FtpWorker::scaner_FTP(const std::function<void(const GroupTask&)>& func, const TaskParams& dir)
{
    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();
    if (curl)
    {
        //std::string destination_fix = curl_easy_escape(curl, from.path.u8string().data(), from.path.u8string().size());
        std::string url = fmt::format("ftp://{}", dir.from.entrypoint.value_or(""));
        curl_easy_setopt(curl, CURLOPT_WILDCARDMATCH, 1L);
        curl_easy_setopt(curl, CURLOPT_USERPWD, (dir.from.ftp_user_password.value_or("anonimus:anonimus").data())); // Авторизация на FTP
        curl_easy_setopt(curl, CURLOPT_CHUNK_BGN_FUNCTION, file_is_coming);
        // Выполнение запроса
        if (!recursiv_list_FTP(curl, res, url, dir.from.path.u8string(), func,dir))
        {
            curl_easy_cleanup(curl);
            return false;
        }
        curl_easy_cleanup(curl);
        return true;
    }
    else
    {
        spdlog::error("Curl init error");
        return false;
    }
    
}

bool FtpWorker::file_upload_curl(const SimpleTask& task)
{
    
	CURL* curl;
	CURLcode res;
	curl = curl_easy_init();

	if (curl) {
		auto file = std::ifstream(task.m_source, std::ios::binary);
		if (!file.is_open()) {
			spdlog::error("Не удалось открыть файл для чтения: {}", task.m_source.u8string());
			//m_file_monitor->count_error.fetch_add(1, std::memory_order_release);
			return false;
		}

		// Полный URL для файла на сервере FTP

		auto destination = task.m_to.path / task.m_relative_path;
		std::string destination_fix = destination.u8string();
		std::transform(destination_fix.begin(), destination_fix.end(), destination_fix.begin(), [](unsigned char c) {
			return (c == '\\') ? '/' : c;
			});
		destination_fix = curl_easy_escape(curl, destination_fix.c_str(), 0);
		auto full_url = fmt::format("ftp://{}/{}", task.m_to.entrypoint.value(), destination_fix);

		// Устанавливаем параметры libcurl
		curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
		curl_easy_setopt(curl, CURLOPT_USERPWD, (task.m_to.ftp_user_password.value().data())); // Авторизация на FTP
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);                    // Устанавливаем режим загрузки
		curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 128 * 1024L);  // Размер буфера для передачи данных (1 KB)
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);    // Устанавливаем функцию обратного вызова для чтения данных
		curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, (long)CURLFTP_CREATE_DIR_RETRY);
		curl_easy_setopt(curl, CURLOPT_READDATA, &file);
		//	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
			// Определяем размер файла
		file.seekg(0, std::ios::end);
		curl_off_t fsize = file.tellg();
		if (fsize < 0) {
			spdlog::error("Не удалось определить размер файла: {}", task.m_source.u8string());
			//m_file_monitor->count_error.fetch_add(1, std::memory_order_release);
			file.close();
			curl_easy_cleanup(curl);
			return false;
		}
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, fsize); // Размер файла

		// Возвращаемся к началу файла
		file.seekg(0, std::ios::beg);  // Возвращаемся к началу файла

		// Выполняем загрузку
		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			spdlog::error("Ошибка при загрузке на FTP: {}", curl_easy_strerror(res));
			//m_file_monitor->count_error.fetch_add(1, std::memory_order_release);
			file.close();
			curl_easy_cleanup(curl);
			return false;
		}

		// Завершаем работу с curl
		file.close();
		curl_easy_cleanup(curl);
		return true;
	}
	else
	{
		spdlog::error("Не удалось инициализировать curl");
		return false;
	}
	return true;
}

bool FtpWorker::file_download_curl(const SimpleTask& task)
{
	CURL* curl;
	CURLcode res;
	curl = curl_easy_init();

	if (curl) {
		auto destination = task.m_to.path / task.m_relative_path;
		try
		{
			fs::create_directories(destination.parent_path());
		}
		catch (fs::filesystem_error& e)
		{
			//m_file_monitor->count_error.fetch_add(1, std::memory_order_release);
			spdlog::error("Create directory error: {}", to_utf8(e.what()));
		}
		// Открываем целевой файл для записи
		std::ofstream file(destination, std::ios::binary);
		if (!file.is_open()) {
			spdlog::error("Не удалось открыть файл для записи: {}", destination.u8string());
			return false;
		}
		std::string temp_subfolder = fixSlashesInURL(curl_easy_escape(curl, task.m_source.u8string().data(), task.m_source.u8string().size()));
		// Формируем URL для FTP
		std::string full_url = fmt::format("ftp://{}{}", task.m_from.entrypoint.value(), temp_subfolder);
		// Устанавливаем параметры cURL
		curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
		curl_easy_setopt(curl, CURLOPT_USERPWD, task.m_from.ftp_user_password.value().data()); // Авторизация
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);  // Функция записи данных
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);             // Локальный файл для записи данных
		//curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, (long)CURLFTP_CREATE_DIR_RETRY); // Попытки создания директорий

		// Выполняем скачивание
		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			spdlog::error("Ошибка при скачивании с FTP: {}", curl_easy_strerror(res));
			file.close();
			curl_easy_cleanup(curl);
			return false;
		}

		// Завершаем работу с curl
		file.close();
		curl_easy_cleanup(curl);
		return true;
	}
	return false;

}
bool FtpWorker::remove(const SimpleTask& task)
{
    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();

    if (curl) {

        std::string temp_subfolder = fixSlashesInURL(curl_easy_escape(curl, task.m_source.u8string().data(), task.m_source.u8string().size()));
        // Формируем URL для FTP
        std::string full_url = fmt::format("ftp://{}{}", task.m_from.entrypoint.value(), temp_subfolder);

        // Устанавливаем параметры cURL
        curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERPWD, task.m_from.ftp_user_password.value().data()); // Авторизация

        // Настройка команды для удаления файла
        struct curl_slist* headerlist = nullptr;
        std::string file_name = task.m_source.filename().u8string();
        file_name = curl_easy_escape(curl, file_name.data(), file_name.size());
        headerlist = curl_slist_append(headerlist, fmt::format("DELE {}", file_name).c_str());
        curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);

        //// Устанавливаем команду удаления
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        // Выполняем скачивание
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            spdlog::error("Ошибка при удалении с FTP: {}", curl_easy_strerror(res));
            curl_slist_free_all(headerlist);  // Освобождаем список команд
            curl_easy_cleanup(curl);
            return false;
        }
        curl_slist_free_all(headerlist);  // Освобождаем список команд
        curl_easy_cleanup(curl);
        return true;
    }
}
bool FtpWorker::ftp_to_dir(const SimpleTask& task, const bool& only_move)
{
    auto destination = task.m_to.path / task.m_relative_path;
    if (file_download_curl(task))
    {
        if (only_move)
        {
            remove(task);

            spdlog::info("Move file from:{} to:{}", task.m_source.u8string(), destination.u8string());
        }
        else
        {
            spdlog::info("Copy file from:{} to:{}", task.m_source.u8string(), destination.u8string());
        }

    }
    else
    {
        if (only_move)
        {
            spdlog::info("Error file from:{} to:{}", task.m_source.u8string(), destination.u8string());
        }
        else
        {
            spdlog::info("Error file from:{} to:{}", task.m_source.u8string(), destination.u8string());
        }
        //m_file_monitor->count_complite.fetch_add(1, std::memory_order_release);
        //m_file_monitor->count_error.fetch_add(1, std::memory_order_release);
        return false;
    }
}