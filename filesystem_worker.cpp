#include "filesystem_worker.h"
bool FilesystemWorker::scaner_dirs(const std::function<void(const GroupTask&)>& func,const TaskParams& dir)
{
    try {

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
        auto it = fs::recursive_directory_iterator(dir.from.path);
        if (!dir.recursiv)
            it.disable_recursion_pending();
        for (const auto& entry : it) {
            if (fs::is_regular_file(entry.path()) && !fs::is_block_file(entry.path()))
            {
                if (dir.from.mask.has_value() && !RE2::FullMatch(entry.path().filename().u8string(), mask_re2.value()))
                {
                    continue;
                }
                GroupTask group_task(dir.id);
                fs::path relative_path = fs::relative(entry.path(), dir.from.path);
                auto replace_filename = [&](const UrlParams& p) {
                    if (dir.from.mask.has_value() && p.mask.has_value())
                    {
                        auto filename = relative_path.filename().u8string();
                        RE2::Replace(&filename, mask_re2.value(), p.mask.value());
                        relative_path.replace_filename(fs::u8path(filename));
                    }
                    };
                if (dir.to.size() == 1)
                {
                    group_task.set_file_mode(FileMode::ONLY_MOVE);
                    replace_filename(dir.to.back());
                    group_task.push_task(dir.from, dir.to.back(), relative_path, entry.path());
                }
                else if (dir.file_mode == FileMode::BALANCER)
                {
                   // for (const auto& path_to : dir.to) {
                        round_robin %= dir.to.size();
                        group_task.set_file_mode(FileMode::BALANCER);
                        replace_filename(dir.to.at(round_robin));
                        group_task.push_task(dir.from, dir.to.at(round_robin), relative_path, entry.path());
                        ++round_robin;
                    //}
                }
                else {

                    for (const auto& path_to : dir.to) {
                        
                        //auto destination = path_to.path / relative_path;
                        group_task.set_file_mode(FileMode::COPY);
                        replace_filename(path_to);
                        group_task.push_task(dir.from, path_to, relative_path, entry.path());

                    }
                }
                func(group_task);
            }
            //else if (fs::is_directory(entry.path()) && fs::is_empty(entry.path()))
            //{
            //    remove_empty_directory(entry.path());
            //}

        }
        
    }

    catch (fs::filesystem_error& e)
    {
        spdlog::error("Scan dirrectory error:{}", to_utf8(e.what()));
        return false;
    }
    return true;
}
bool FilesystemWorker::fs_to_fs(const SimpleTask& task,const bool& only_move)
{
    try
    {
        fs::path destination = task.m_to.path / task.m_relative_path;
        fs::create_directories(destination.parent_path());
        if (only_move)
        {
            fs::rename(task.m_source, destination);
            spdlog::info("Move file from:{} to:{}", task.m_source.u8string(), destination.u8string());
        }
        else
        {
            fs::copy_file(task.m_source, destination, std::filesystem::copy_options::overwrite_existing);
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
//void FilesystemWorker::remove_empty_directory(const fs::path& dir)
//{
//    auto ftime = fs::last_write_time(dir);
//    auto now = std::chrono::system_clock::now();
//    auto last_write_time = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
//        ftime - fs::file_time_type::clock::now() + now
//    );
//    auto age_in_days = std::chrono::duration_cast<std::chrono::hours>(now - last_write_time).count() / 24;
//    if (age_in_days >= 2) {
//        try
//        {
//            fs::remove(dir);
//            spdlog::error("Remove directory:{}", dir.u8string());
//
//        }
//        catch (fs::filesystem_error& e) {
//            spdlog::error("Error remove directory: {}", to_utf8(e.what()));
//        }
//    }
//}
bool FilesystemWorker::remove(const fs::path& source)
{
    try
    {
        fs::remove(source);
        spdlog::info("Remove file:{}", source.u8string());
        
    }
    catch (fs::filesystem_error& e)
    {
        spdlog::error("Remove file error:{}", to_utf8(e.what()));
        return false;
    }
    return true;
}
bool FilesystemWorker::fs_to_ftp(const SimpleTask& task, const bool& only_move)
{
    auto destination = task.m_to.path / task.m_relative_path;
    
    if (FtpWorker::file_upload_curl(task.m_to,task.m_source, destination))
    {
        if (only_move)
        {
            if (!remove(task.m_source))
            {
                //m_file_monitor->count_complite.fetch_add(1, std::memory_order_release);
                //m_file_monitor->count_error.fetch_add(1, std::memory_order_release);
                return false;
            }

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
    return true;
}
bool FilesystemWorker::fs_to_s3(const SimpleTask& task, const bool& only_move)
{
    auto destination = task.m_to.path / task.m_relative_path;

    if (S3Worker::file_upload(task.m_to,task.m_source,destination))
    {
        if (only_move)
        {
            if (!remove(task.m_source))
            {
                //m_file_monitor->count_complite.fetch_add(1, std::memory_order_release);
                //m_file_monitor->count_error.fetch_add(1, std::memory_order_release);
                return false;
            }

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
    return true;
}