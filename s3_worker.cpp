#include "s3_worker.h"
bool S3Worker::remove(const SimpleTask& task)
{
    Aws::Client::ClientConfiguration config;
    // config.region = Aws::String("ru-central1");
    config.allowSystemProxy = true;
    config.verifySSL = false;
    if (task.m_from.entrypoint.has_value())
        config.endpointOverride = task.m_from.entrypoint.value();
    Aws::Auth::AWSCredentials cred;
    if (task.m_from.access_key.has_value())
        cred.SetAWSAccessKeyId(task.m_from.access_key.value());
    if (task.m_from.secret_key.has_value())
        cred.SetAWSSecretKey(task.m_from.secret_key.value());

    Aws::S3::S3Client s3_client(cred, nullptr, config);
    // Создаем запрос на получение объекта из S3
    Aws::S3::Model::DeleteObjectRequest request;
    request.SetBucket(task.m_from.bucket.value());  // Имя бакета
    request.SetKey(task.m_source.u8string());     // Ключ объекта (путь к файлу в бакете)
    //fs::path destination = task.m_to.path / task.m_relative_path;
    auto outcome = s3_client.DeleteObject(request);
    
    if (outcome.IsSuccess()) {
        spdlog::info("Sucsess remove object : {}", task.m_source.u8string());
        return true;
    }
    else {
        spdlog::error("Error remove object : {}", outcome.GetError().GetMessage());
        return false;
    }
    return true;
}
bool S3Worker::file_upload(const UrlParams& to, const fs::path& source, const fs::path& destination)
{
    // Verify that the file exists.
    Aws::Client::ClientConfiguration config;
    // config.region = Aws::String("ru-central1");
    config.allowSystemProxy = true;
    config.verifySSL = false;
    if (to.entrypoint.has_value())
        config.endpointOverride = to.entrypoint.value();
    Aws::Auth::AWSCredentials cred;
    if (to.access_key.has_value())
        cred.SetAWSAccessKeyId(to.access_key.value());
    if (to.secret_key.has_value())
        cred.SetAWSSecretKey(to.secret_key.value());

    Aws::S3::S3Client s3_client(cred, nullptr, config);

    Aws::S3::Model::PutObjectRequest request;
    if(to.bucket.has_value())
    request.SetBucket(to.bucket.value());
    std::string destination_fix = destination.u8string();
    std::transform(destination_fix.begin(), destination_fix.end(), destination_fix.begin(), [](unsigned char c) {
        return (c == '\\') ? '/' : c;
        });
    request.SetKey(destination_fix);

    std::shared_ptr<Aws::IOStream> input_data =
        Aws::MakeShared<Aws::FStream>("SampleAllocationTag",
            source,
            std::ios_base::in | std::ios_base::binary);

    request.SetBody(input_data);

    Aws::S3::Model::PutObjectOutcome outcome =
        s3_client.PutObject(request);

    if (outcome.IsSuccess()) {
        spdlog::trace("Successfully upload {} to S3.", destination.u8string());
        return true;
    }
    else
    {
        spdlog::error("Error downloading object : {}", outcome.GetError().GetMessage());
        return false;
    }
    return true;
}
bool S3Worker::file_download(const UrlParams& from, const fs::path& source,const fs::path& destination )
{
    Aws::Client::ClientConfiguration config;
    // config.region = Aws::String("ru-central1");
    config.allowSystemProxy = true;
    config.verifySSL = false;
    if (from.entrypoint.has_value())
        config.endpointOverride = from.entrypoint.value();
    Aws::Auth::AWSCredentials cred;
    if (from.access_key.has_value())
        cred.SetAWSAccessKeyId(from.access_key.value());
    if (from.secret_key.has_value())
        cred.SetAWSSecretKey(from.secret_key.value());

    Aws::S3::S3Client s3_client(cred, nullptr, config);
    // Создаем запрос на получение объекта из S3
    Aws::S3::Model::GetObjectRequest request;
    request.SetBucket(from.bucket.value());  // Имя бакета
    request.SetKey(source.u8string());     // Ключ объекта (путь к файлу в бакете)

    request.SetResponseStreamFactory([=]() {return Aws::New<std::fstream>("S3DownloadBuffer", destination, std::ios_base::out | std::ios_base::in | std::ios_base::trunc | std::ios_base::binary); });
    auto outcome = s3_client.GetObject(request);
    if (outcome.IsSuccess()) {
        
        spdlog::trace("Successfully downloaded {} from S3.", source.u8string());
    }
    else {
        spdlog::error("Error downloading object : {}", outcome.GetError().GetMessage());
        return false;
    }

    return true;
}
bool S3Worker::s3_to_fs(const SimpleTask& task, const bool& only_move)
{


    try
    {
        fs::path destination = task.m_to.path / task.m_relative_path;

        try
        {
            fs::create_directories(destination.parent_path());
        }
        catch (fs::filesystem_error& e)
        {

            spdlog::error("Create directory error:{}", to_utf8(e.what()));
            return false;
        }

        if (only_move)
        {
            file_download(task.m_from,task.m_source,destination);
            remove(task);
            //fs::rename(task.m_source, destination);
            spdlog::info("Move file from:{} to:{}", task.m_source.u8string(), destination.u8string());
        }
        else
        {
            file_download(task.m_from, task.m_source, destination);
            //fs::copy_file(task.m_source, destination, std::filesystem::copy_options::overwrite_existing);
            spdlog::info("Copy file from:{} to:{}", task.m_source.u8string(), destination.u8string());
        }


    }
    catch (fs::filesystem_error& e)
    {
        //m_file_monitor->count_error.fetch_add(1, std::memory_order_release);
        if (only_move)
            spdlog::error("Move file error:{}", to_utf8(e.what()));
        else
            spdlog::error("Copy file error: {}", to_utf8(e.what()));
        //m_file_monitor->count_complite.fetch_add(1, std::memory_order_release);
        //m_file_monitor->count_error.fetch_add(1, std::memory_order_release);
        return false;
    }
    return true;
}
bool S3Worker::s3_to_ftp(const SimpleTask& task, const bool& only_move)
{
    fs::path destination = task.m_to.path / task.m_relative_path;
    char temp_path[MAX_PATH];
    char temp_file[MAX_PATH];


    // Генерируем временное имя файла
    if (GetTempFileName(fs::temp_directory_path().u8string().c_str(), "TMP_MOVER", 0, temp_file) == 0) {
        spdlog::error("Failed to create temporary file name.");
        return false;
    }
    if (only_move)
    {
        file_download(task.m_from, task.m_source, temp_file);
        FtpWorker::file_upload_curl(task.m_to, temp_file, destination);
        remove(task);
        //fs::rename(task.m_source, destination);
        spdlog::info("Move file from:{} to:{}", task.m_source.u8string(), destination.u8string());
    }
    else
    {
        file_download(task.m_from, task.m_source, temp_file);
        FtpWorker::file_upload_curl(task.m_to, temp_file, destination);
        //fs::copy_file(task.m_source, destination, std::filesystem::copy_options::overwrite_existing);
        spdlog::info("Copy file from:{} to:{}", task.m_source.u8string(), destination.u8string());
    }
    DeleteFile(temp_file);
    return true;
}
bool S3Worker::s3_to_s3(const SimpleTask& task, const bool& only_move)
{


    try
    {
        fs::path destination = task.m_to.path / task.m_relative_path;
        char temp_path[MAX_PATH];
        char temp_file[MAX_PATH];
        
        
        // Генерируем временное имя файла
        if (GetTempFileName(fs::temp_directory_path().u8string().c_str(), "TMP_MOVER", 0, temp_file) == 0) {
            spdlog::error("Failed to create temporary file name.");
            return false;
        }
        if (only_move)
        {
            file_download(task.m_from, task.m_source, temp_file);
            file_upload(task.m_to, temp_file, destination);
            remove(task);
            //fs::rename(task.m_source, destination);
            spdlog::info("Move file from:{} to:{}", task.m_source.u8string(), destination.u8string());
        }
        else
        {
            file_download(task.m_from, task.m_source, temp_file);
            file_upload(task.m_to, temp_file, destination);
            //fs::copy_file(task.m_source, destination, std::filesystem::copy_options::overwrite_existing);
            spdlog::info("Copy file from:{} to:{}", task.m_source.u8string(), destination.u8string());
        }
        DeleteFile(temp_file);

    }
    catch (fs::filesystem_error& e)
    {
        //m_file_monitor->count_error.fetch_add(1, std::memory_order_release);
        if (only_move)
            spdlog::error("Move file error:{}", to_utf8(e.what()));
        else
            spdlog::error("Copy file error: {}", to_utf8(e.what()));
        //m_file_monitor->count_complite.fetch_add(1, std::memory_order_release);
        //m_file_monitor->count_error.fetch_add(1, std::memory_order_release);
        return false;
    }
    return true;
}
bool S3Worker::scaner_S3(const std::function<void(const GroupTask&)>& func, const TaskParams& dir)
{
    Aws::Client::ClientConfiguration config;
   // config.region = Aws::String("ru-central1");
    config.allowSystemProxy=true;
    config.verifySSL = false;
    if(dir.from.entrypoint.has_value())
        config.endpointOverride=dir.from.entrypoint.value();
    Aws::Auth::AWSCredentials cred;
    if (dir.from.access_key.has_value())
        cred.SetAWSAccessKeyId(dir.from.access_key.value());
    if (dir.from.secret_key.has_value())
        cred.SetAWSSecretKey(dir.from.secret_key.value());

    Aws::S3::S3Client s3_client(cred,nullptr,config);
    Aws::S3::Model::ListObjectsV2Request request;
    //Aws::S3::Model::ListBucketsOutcome outcome = s3_client.ListBuckets();
    //Aws::Vector<Aws::S3::Model::Bucket> bucket_list = outcome.GetResult().GetBuckets();
    if (dir.from.bucket.has_value())
        request.SetBucket(dir.from.bucket.value());
    request.SetPrefix(dir.from.path.u8string());
    if (!dir.recursiv)
        request.SetDelimiter("/");
    Aws::String continuation_token;
    std::optional<RE2> mask_re2;
    if (dir.from.mask.has_value())
    {
        mask_re2.emplace(dir.from.mask.value());
        if (!mask_re2.value().ok())
        {
            spdlog::error("Error regular expression in {}: {}", dir.from.mask.value(), mask_re2.value().error());
            return false;
        }
    }
    auto round_robin = 0;
    do {
        // Если есть токен продолжения, устанавливаем его
        if (!continuation_token.empty()) {
            request.SetContinuationToken(continuation_token);
        }

        // Выполнение запроса
        auto outcome = s3_client.ListObjectsV2(request);
        
        if (outcome.IsSuccess()) {
            // Обработка результата
            const auto& objects = outcome.GetResult().GetContents();
            for (const auto& object : objects) {
                fs::path entry_path = fs::u8path(object.GetKey());
                fs::path relative_path = fs::relative(entry_path, dir.from.path);
                auto replace_filename = [&](const UrlParams& p) {
                    if (dir.from.mask.has_value() && p.mask.has_value())
                    {
                        auto filename = relative_path.filename().u8string();
                        RE2::Replace(&filename, mask_re2.value(), p.mask.value());
                        relative_path.replace_filename(fs::u8path(filename));
                    }
                    };
                if (dir.from.mask.has_value() && !RE2::FullMatch(entry_path.filename().u8string(), mask_re2.value()))
                {
                    continue;
                }
                GroupTask group_task(dir.id);
                if (dir.to.size() == 1)
                {
                    group_task.set_file_mode(FileMode::ONLY_MOVE);
                    replace_filename(dir.to.back());
                    group_task.push_task(dir.from, dir.to.back(), relative_path, entry_path);
                }
                else if (dir.file_mode == FileMode::BALANCER)
                {
                    // for (const auto& path_to : dir.to) {
                    round_robin %= dir.to.size();
                    group_task.set_file_mode(FileMode::BALANCER);
                    replace_filename(dir.to.at(round_robin));
                    group_task.push_task(dir.from, dir.to.at(round_robin), relative_path, entry_path);
                    ++round_robin;
                    //}
                }
                else {

                    for (const auto& path_to : dir.to) {

                        //auto destination = path_to.path / relative_path;
                        group_task.set_file_mode(FileMode::COPY);
                        replace_filename(path_to);
                        group_task.push_task(dir.from, path_to, relative_path, entry_path);

                    }
                }
                func(group_task);
            }

            // Получение токена продолжения
            continuation_token = outcome.GetResult().GetNextContinuationToken();
        }
        else {
            spdlog::error("Error: {}", outcome.GetError().GetMessage());    
            return false;
        }
    } while (!continuation_token.empty()); // Повторяем, пока есть токен продолжения
    return true;
}