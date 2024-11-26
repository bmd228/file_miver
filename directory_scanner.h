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

namespace fs = std::filesystem;
using namespace std::chrono_literals;

class DirectoryScanner {
public:
    DirectoryScanner(boost::asio::io_context& io, const std::vector<TaskParams>& directories, std::chrono::seconds interval)
        : io_(io), directories_(directories), timer_(io),
        interval_(interval), active_tasks_(0) {}

    void start() {
        schedule_scan();
    }
    void process_file(const GroupTask& gr_task);
private:

    void schedule_scan();

    void scan_directories();

    void delete_file(const SimpleTask& task);

    void check_completion();

    boost::asio::io_context& io_;
    std::vector<TaskParams>  directories_;
    boost::asio::steady_timer timer_;
    std::chrono::seconds interval_;
    std::atomic<int> active_tasks_;

};