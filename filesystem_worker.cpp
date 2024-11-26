#include "filesystem_worker.h"
bool FilesystemWorker::scaner_dirs(const std::function<void(const GroupTask&)>& func,const TaskParams& dir)
{
    try {
        auto it = fs::recursive_directory_iterator(dir.from.path);
        if (!dir.recursiv)
            it.disable_recursion_pending();
        for (const auto& entry : it) {
            if (fs::is_regular_file(entry.path()) && !fs::is_block_file(entry.path()))
            {
                GroupTask group_task;
                fs::path relative_path = fs::relative(entry.path(), dir.from.path);
                if (dir.to.size() == 1)
                {
                    group_task.set_file_mode(FileMode::ONLY_MOVE);
                    group_task.push_task(dir.from, dir.to.back(), relative_path, entry.path());
                }
                else if (dir.file_mode == FileMode::BALANCER)
                {
                    for (const auto& path_to : dir.to) {
                        group_task.set_file_mode(FileMode::BALANCER);
                        group_task.push_task(dir.from, dir.to.back(), relative_path, entry.path());
                    }
                }
                else {

                    for (const auto& path_to : dir.to) {
                        //auto destination = path_to.path / relative_path;
                        group_task.set_file_mode(FileMode::COPY);
                        group_task.push_task(dir.from, path_to, relative_path, entry.path());

                    }
                }
                func(group_task);
            }

        }

    }
    catch (fs::filesystem_error& e)
    {
        spdlog::error("Scan dirrectory error:{}", to_utf8(e.what()));
        return false;
    }
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
}
bool FilesystemWorker::remove(const fs::path& source)
{
    try
    {
        fs::remove(source);
        spdlog::info("Remove file:{}", source.u8string());
        return true;
    }
    catch (fs::filesystem_error& e)
    {
        spdlog::error("Remove file error:{}", to_utf8(e.what()));
        return false;
    }
}
bool FilesystemWorker::fs_to_ftp(const SimpleTask& task, const bool& only_move)
{
    auto destination = task.m_to.path / task.m_relative_path;
    
    if (FtpWorker::file_upload_curl(task))
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
}