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
static std::string enum_to_string(Protocol protocol) {
    switch (protocol) {
    case Protocol::DIR:   return "DIR";
    case Protocol::FTP: return "FTP";
    case Protocol::S3:  return "S3";
    default:           return "Unknown";
    }
}
int main() {
    std::shared_ptr<spdlog::logger> logger = init_logger();

    boost::asio::io_context io;
//Обязательно все директории должны заканчиваться на / и добавить провреку на win style и linux style 
    TaskParams one_task, two_task,three_task, four_task,five_task, six_task,seven_task;
    one_task.set_from(fs::path("/test/"), Protocol::FTP, ".*", "keenetic-0771", "user:user");
    one_task.add_to(fs::path("D:\\test\\"), Protocol::DIR, ".*");
    one_task.recursiv = true;
    one_task.file_mode = FileMode::COPY;

    two_task.from = UrlParams(fs::path("D:\\test\\1\\242424"), Protocol::DIR, std::nullopt);
    two_task.to = { UrlParams(fs::path("D:\\test\\2\\"), Protocol::DIR, std::nullopt),UrlParams(fs::path("/test/"), Protocol::FTP, std::nullopt,"keenetic-0771","user:user") };
    two_task.add_to("", Protocol::S3, std::nullopt, "http://localhost:9000", std::nullopt, "x6poJxUuRH14X7Co4aEz", "mCpPWDIYBzKC2mM8TIhqEnrWrjzspNPUnulIT2Yo", "/rabbit");
    two_task.recursiv = true;
    two_task.file_mode = FileMode::COPY;

    three_task.set_from(fs::path("/test/"), Protocol::FTP, ".*", "keenetic-0771", "user:user");
    three_task.add_to(fs::path("/test2/"), Protocol::FTP, ".*", "keenetic-0771", "user:user");
    three_task.recursiv = true;
    three_task.file_mode = FileMode::COPY;

    four_task.from = UrlParams(fs::path("D:\\test\\1\\"), Protocol::DIR, "(.*\\.)(bin)");
    four_task.to = { UrlParams(fs::path("D:\\test\\2\\"), Protocol::DIR, "\\1call"),UrlParams(fs::path("/test/"), Protocol::FTP, ".*","keenetic-0771","user:user") };
    four_task.recursiv = true;
    four_task.file_mode = FileMode::COPY;

    five_task.from = UrlParams(fs::path("D:\\test\\1\\"), Protocol::DIR, ".*");
    five_task.to = { UrlParams(fs::path("D:\\test\\2\\")),UrlParams(fs::path("/test/"), Protocol::FTP,std::nullopt,"keenetic-0771","user:user") };
    five_task.recursiv = true;
    five_task.file_mode = FileMode::BALANCER;

    six_task.set_from("", Protocol::S3, std::nullopt, "http://localhost:9000", std::nullopt, "x6poJxUuRH14X7Co4aEz", "mCpPWDIYBzKC2mM8TIhqEnrWrjzspNPUnulIT2Yo", "/rabbit");
    six_task.add_to(fs::path("D:\\test\\"), Protocol::DIR, ".*");
    six_task.recursiv = true;
    six_task.file_mode = FileMode::COPY;

    seven_task.set_from(fs::path("/test/"), Protocol::FTP, ".*", "keenetic-0771", "user:user");
    seven_task.add_to("/2/", Protocol::S3, std::nullopt, "http://localhost:9000", std::nullopt, "x6poJxUuRH14X7Co4aEz", "mCpPWDIYBzKC2mM8TIhqEnrWrjzspNPUnulIT2Yo", "/rabbit");
    seven_task.recursiv = true;
    seven_task.file_mode = FileMode::COPY;



    DirectoryScanner scanner( { two_task,seven_task });
    scanner.start();
    auto pair_id = scanner.get_id_params();
    while (true)
    {
        for (const auto& dir : scanner.monitor)
        {
            fmt::print("RUN:{} {}:{} COMPLITE:{} ERROR:{} \r\n", dir.second->is_run.load(), enum_to_string(pair_id[dir.first].from.protocol), pair_id[dir.first].from.path.u8string(), dir.second->sucsess.load(), dir.second->error.load());
        }
        std::this_thread::sleep_for(1s);
    }
    
    //while (true) {
    //    //std::cout << "> ";

    //    //if (!std::getline(std::cin, input)) { // Обработка EOF
    //    //    scanner.stop();
    //    //    std::cout << "\nCtrl+Z/Ctrl+D обнаружен. Завершаем работу...\n";
    //    //    break;
    //    //}
    //}

    //std::cout << "Scanner stopped.\n";
    
    // Запуск io_context в пуле потоков
  //  std::vector<std::thread> threads;
   // const int num_threads = std::thread::hardware_concurrency();
    //for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
    //    threads.emplace_back([&io]() { io.run(); });
    //}

    

    return 0;
}
