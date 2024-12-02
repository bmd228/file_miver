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
    DirectoryScanner(const std::vector<TaskParams>& directories)
        :  directories_(directories)
    {
        curl_global_init(CURL_GLOBAL_ALL);
        
        Aws::InitAPI(options);
        for (const auto& dir : directories_)
        {
            active_tasks_.emplace(dir.id, 0);
            timer_.emplace(dir.id, std::make_shared<boost::asio::steady_timer>(io_));
            monitor.emplace(dir.id, std::make_shared<Monitor>());
        }

    }
    ~DirectoryScanner()
    {
        stop();
        curl_global_cleanup();
        Aws::ShutdownAPI(options);
    }
    void start() {
        for (const auto& dir : directories_)
        {
            io_.post([&]() {
                scan_directories(dir.id);
                });
        }
            
            //schedule_scan(dir.id);
        for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
            threads.emplace_back([&]() { io_.run(); });
        }
    }
    void stop()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            io_.stop();
        }

        // Уведомляем, когда io_context остановится
        std::thread([this] {
            io_.run(); // Это завершится, когда все обработчики будут выполнены.
            std::lock_guard<std::mutex> lock(mutex_);
            stopped = true;
            cv_.notify_one();
            }).detach();

        // Ожидаем, пока io_context полностью остановится
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { return stopped; });
            }

            // Ждем завершения потоков
            for (auto& thread : threads) {
                if (thread.joinable()) 
                    thread.join();
            }
            spdlog::info("Scanner stopped");
    }
    void process_file(const GroupTask& gr_task);
    std::unordered_map<size_t, TaskParams> get_id_params()
    {
        std::unordered_map<size_t, TaskParams> pair_id;
        for (const auto& dir : directories_)
        {
            pair_id.emplace(dir.id, dir);
        }
        return pair_id;
    }
    std::unordered_map<size_t, std::shared_ptr<Monitor>> monitor;
private:

    void schedule_scan(const size_t& id);

    void scan_directories(const size_t& id);

    void delete_file(const SimpleTask& task);

    void check_completion(const size_t& id);

    Aws::SDKOptions options;
    boost::asio::io_context io_;
    std::vector<std::thread> threads;
    std::vector<TaskParams>  directories_;
    std::unordered_map<size_t, std::shared_ptr<boost::asio::steady_timer>> timer_;
    std::unordered_map<size_t, std::atomic_int> active_tasks_;

    std::mutex mutex_; 
    std::condition_variable cv_;
    bool stopped = false;

    


};