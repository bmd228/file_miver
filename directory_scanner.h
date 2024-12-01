#pragma once
#include <iostream>
#include <boost/asio.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <atomic>
#include <chrono>
#include <thread>

#include <functional>

#include "task_params.h"
#include "filesystem_worker.h"
#include "s3_worker.h"

namespace fs = std::filesystem;
using namespace std::chrono_literals;

class DirectoryScanner {
public:
    DirectoryScanner(boost::asio::io_context& io, const std::vector<TaskParams>& directories, std::chrono::seconds interval)
        : io_(io), directories_(directories), interval_(interval)
    {
        for (const auto& dir : directories_)
        {
            active_tasks_.emplace(dir.id, 0);
            timer_.emplace(dir.id, std::make_shared<boost::asio::steady_timer>(io));
        }

    }

    void start() {
        for (const auto& dir : directories_)
            schedule_scan(dir.id);
    }
    void process_file(const GroupTask& gr_task);
private:

    void schedule_scan(const size_t& id);

    void scan_directories(const size_t& id);

    void delete_file(const SimpleTask& task);

    void check_completion(const size_t& id);

    boost::asio::io_context& io_;
    std::vector<TaskParams>  directories_;
    std::chrono::seconds interval_;
    std::unordered_map<size_t, std::shared_ptr<boost::asio::steady_timer>> timer_;
    std::unordered_map<size_t, std::atomic_int> active_tasks_;



};