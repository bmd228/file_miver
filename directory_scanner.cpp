#include "directory_scanner.h"

void DirectoryScanner::process_file(const GroupTask& gr_task) {

    auto pending_operations = std::make_shared<std::atomic<int>>(gr_task.m_group_task.size());
    active_tasks_[gr_task.m_id].fetch_add(gr_task.m_group_task.size() + 1, std::memory_order_relaxed);

    for (const auto& task : gr_task.m_group_task) {
        io_.post([this, task, pending_operations, only_move = gr_task.m_file_mode == FileMode::ONLY_MOVE ? true : false]() {
            auto status = false;
            switch (task.m_from.protocol)
            {
            case Protocol::DIR:
            {
                switch (task.m_to.protocol)
                {
                case Protocol::DIR:
                {
                    status=FilesystemWorker::fs_to_fs(task, only_move);
                }
                break;
                case Protocol::FTP:
                {
                    status=FilesystemWorker::fs_to_ftp(task, only_move);
                }
                break;
                case Protocol::S3:
                {
                    status=FilesystemWorker::fs_to_s3(task, only_move);
                }
                break;
                }
            }
            break;
            case Protocol::FTP:
            {
                switch (task.m_to.protocol)
                {
                case Protocol::DIR:
                {
                    status=FtpWorker::ftp_to_dir(task, only_move);
                }
                break;
                case Protocol::FTP:
                {
                    status=FtpWorker::ftp_to_ftp(task, only_move);
                }
                break;
                case Protocol::S3:
                {
                    status=FtpWorker::ftp_to_s3(task, only_move);
                }
                break;
                }
            }
            break;
            case Protocol::S3:
            {
                switch (task.m_to.protocol)
                {
                case Protocol::DIR:
                {
                    status=S3Worker::s3_to_fs(task, only_move);
                }
                break;
                case Protocol::FTP:
                {
                    status=S3Worker::s3_to_ftp(task, only_move);
                }
                break;

                case Protocol::S3:
                {
                    status=S3Worker::s3_to_s3(task, only_move);
                }
                break;
                }
            }
            break;
            }
            if (!status)
                monitor[task.m_id]->error++;
            else
                monitor[task.m_id]->sucsess++;
            if (--(*pending_operations) == 0) {
                if (!only_move)
                    delete_file(task);
                else
                {
                    active_tasks_[task.m_id].fetch_sub(1, std::memory_order_relaxed);
                    
                }
            }
            active_tasks_[task.m_id].fetch_sub(1, std::memory_order_relaxed);
            check_completion(task.m_id);
            });
    }
}
void DirectoryScanner::delete_file(const SimpleTask& task) {
    io_.post([this, task]() {
        switch (task.m_from.protocol)
        {
        case Protocol::DIR:
        {
            FilesystemWorker::remove(task.m_source);
        }
        break;
        case Protocol::FTP:
        {
            FtpWorker::remove(task);
        }
        break;
        case Protocol::S3:
        {
            S3Worker::remove(task);
        }
        break;
        }
        active_tasks_[task.m_id].fetch_sub(1, std::memory_order_relaxed);
        check_completion(task.m_id);
        });
}

void DirectoryScanner::check_completion(const size_t& id) {
    if (active_tasks_[id].load(std::memory_order_relaxed) == 0) {
        spdlog::trace("All tasks completed id:{}. Scheduling next scan",id);
        schedule_scan(id); // Запускаем новый цикл через таймер
    }
}
void DirectoryScanner::schedule_scan(const size_t& id) {
    timer_[id]->expires_after(10s);
    timer_[id]->async_wait([this, id](const boost::system::error_code& ec) {
        if (!ec) {
            scan_directories(id);
        }
        else {
            spdlog::error("Timer error: {}", to_utf8(ec.message()));
        }
        });
}
void DirectoryScanner::scan_directories(const size_t& id) {
    
    for (const auto& dir : directories_)
    {
        
        if (dir.id != id)
            continue;
        auto status = false;
        spdlog::trace("Starting directory  scan id:{} path: {}", id, dir.from.path.u8string());
        monitor[id]->is_run.exchange(true);
        switch (dir.from.protocol)
        {
        case Protocol::DIR:
        {
            status=FilesystemWorker::scaner_dirs([this](const GroupTask& gr_task) {
                this->process_file(gr_task);
                }, dir);
        }
        break;
        case Protocol::FTP:
        {
            status=FtpWorker::scaner_FTP([this](const GroupTask& gr_task) {
                this->process_file(gr_task);
                }, dir);
        }
        break;
        case Protocol::S3:
        {
            status=S3Worker::scaner_S3([this](const GroupTask& gr_task) {
                this->process_file(gr_task);
                }, dir);
        }
        break;
        }
        monitor[id]->is_run.exchange(false);
        if (!status)
        {
            timer_[id]->expires_after(10min);
            timer_[id]->async_wait([this, id](const boost::system::error_code& ec) {
                if (!ec) {
                    check_completion(id);
                }
                else {
                    spdlog::error("Timer error: {}", to_utf8(ec.message()));
                }
                });
        }
        else
            check_completion(id);
    }
    
}