# CMakeList.txt: проект CMake для CMakeProject6; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#
cmake_minimum_required (VERSION 3.8)

# Включение горячей перезагрузки для компиляторов MSVC, если поддерживается.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("CMakeProject6")
find_package(CURL REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS asio coroutine2)
# Добавьте источник в исполняемый файл этого проекта.
add_executable (CMakeProject6 "main.cpp"  "filesystem_worker.h" "filesystem_worker.cpp"  "utils.h" "task_params.h" "ftp_worker.h" "ftp_worker.cpp" "directory_scanner.h" "directory_scanner.cpp")#
target_link_libraries(CMakeProject6 PRIVATE Boost::coroutine2 Boost::asio)
target_link_libraries(CMakeProject6 PRIVATE spdlog::spdlog_header_only)
target_link_libraries(CMakeProject6 PRIVATE CURL::libcurl )
target_link_libraries(CMakeProject6 PRIVATE fmt::fmt)
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CMakeProject6 PROPERTY CXX_STANDARD 17)
endif()

# TODO: Добавьте тесты и целевые объекты, если это необходимо.
