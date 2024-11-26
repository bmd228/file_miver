#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "directory_scanner.h"

std::shared_ptr<spdlog::logger> init_logger()
{
    //std::setlocale(LC_ALL, "ru_RU.UTF-8");
#ifdef WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif // WIN32
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    //console_sink->set_level(spdlog::level::warn);
    //console_sink->set_pattern("[multi_sink_example] [%^%l%$] %v");

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/multisink.txt", true);
    //file_sink->set_level(spdlog::level::trace);

    std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>("multi_sink", spdlog::sinks_init_list{ console_sink, file_sink });
    logger->set_level(spdlog::level::trace);
    logger->set_pattern("[%H:%M:%S %z] [%n] [%^%l%$] [thread %t] %v");
    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);
    return logger;
}

int main() {
    std::shared_ptr<spdlog::logger> logger = init_logger();

    boost::asio::io_context io;

    TaskParams one_task, two_task;
    one_task.from = UrlParams(fs::path("/test/"), Protocol::FTP, ".*", "keenetic-0771", "user:user");
    one_task.to = { UrlParams(fs::path("D:\\test\\"), Protocol::DIR, ".*") };
    one_task.recursiv = true;
    one_task.file_mode = FileMode::COPY;

    two_task.from = UrlParams(fs::path("D:\\test\\1\\"), Protocol::DIR, ".*");
    two_task.to = { UrlParams(fs::path("D:\\test\\2\\"), Protocol::DIR, ".*"),UrlParams(fs::path("/test/"), Protocol::FTP, ".*","keenetic-0771","user:user") };
    two_task.recursiv = true;
    two_task.file_mode = FileMode::COPY;

    std::chrono::seconds interval = 5s; // Интервал между запусками сканирования

    DirectoryScanner scanner(io, { one_task ,two_task }, interval);
    scanner.start();

    // Запуск io_context в пуле потоков
    std::vector<std::thread> threads;
    const int num_threads = std::thread::hardware_concurrency();
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&io]() { io.run(); });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    return 0;
}
