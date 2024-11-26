#include "directory_scanner.h"

void DirectoryScanner::process_file(const GroupTask& gr_task) {

    auto pending_operations = std::make_shared<std::atomic<int>>(gr_task.m_group_task.size());
    active_tasks_ += gr_task.m_group_task.size() + 1;

    for (const auto& task : gr_task.m_group_task) {
        io_.post([this, task, pending_operations, only_move = gr_task.m_file_mode == FileMode::ONLY_MOVE ? true : false]() {

            switch (task.m_from.protocol)
            {
            case Protocol::DIR:
            {
                switch (task.m_to.protocol)
                {
                case Protocol::DIR:
                {
                    FilesystemWorker::fs_to_fs(task, only_move);
                }
                break;
                case Protocol::FTP:
                {
                    FilesystemWorker::fs_to_ftp(task, only_move);
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
                    FtpWorker::ftp_to_dir(task, only_move);
                }
                break;
                case Protocol::FTP:
                {

                }
                break;
                }
            }
            break;
            }

            if (--(*pending_operations) == 0) {
                if (!only_move)
                    delete_file(task);
            }
            --active_tasks_;
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
        }
        --active_tasks_;
        check_completion();
        });
}

void DirectoryScanner::check_completion() {
    if (active_tasks_ == 0) {
        std::cout << "All tasks completed. Scheduling next scan...\n";
        schedule_scan(); // Запускаем новый цикл через таймер
    }
}
void DirectoryScanner::schedule_scan() {
    timer_.expires_after(interval_);
    timer_.async_wait([this](const boost::system::error_code& ec) {
        if (!ec) {
            scan_directories();
        }
        else {
            std::cerr << "Timer error: " << ec.message() << std::endl;
        }
        });
}
void DirectoryScanner::scan_directories() {
    std::cout << "Starting directory scan...\n";
    for (const auto& dir : directories_)
    {
        switch (dir.from.protocol)
        {
        case Protocol::DIR:
        {
            FilesystemWorker::scaner_dirs([this](const GroupTask& gr_task) {
                this->process_file(gr_task);
                }, dir);
        }
        break;
        case Protocol::FTP:
        {
            FtpWorker::scaner_FTP([this](const GroupTask& gr_task) {
                this->process_file(gr_task);
                }, dir);
        }
        break;
        }
    }
    check_completion();
}