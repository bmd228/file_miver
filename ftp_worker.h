#pragma once
#include <curl/curl.h>
#include <string>
#include <queue>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <fmt/format.h>
#include "utils.h"
#include "task_params.h"
#include "s3_worker.h"

namespace fs = std::filesystem;
struct callback_data {
    struct Info
    {
        curlfiletype type;
        std::string name;
    };
    std::queue<Info> queue_info;
};



static struct FtpWorker
{

    static bool remove(const SimpleTask& task);
    static bool ftp_to_dir(const SimpleTask& task, const bool& only_move);
    static bool ftp_to_ftp(const SimpleTask& task, const bool& only_move);
    static bool ftp_to_s3(const SimpleTask& task, const bool& only_move);
    static bool scaner_FTP(const std::function<void(const GroupTask&)>& func, const TaskParams&);
    static bool file_upload_curl(const UrlParams& to, const fs::path& source, const fs::path& destination);
private:
    static size_t write_callback(void* ptr, size_t size, size_t nmemb, void* stream);
    static size_t read_callback(void* ptr, size_t size, size_t nmemb, void* stream);
    static size_t write_callback_c(void* ptr, size_t size, size_t nmemb, void* stream);
    static size_t read_callback_c(void* ptr, size_t size, size_t nmemb, void* stream);
    static size_t write_callback_list(char* ptr, size_t size, size_t nmemb, void* list);
    static long file_is_coming(struct curl_fileinfo* finfo, struct callback_data* data);
    static bool recursiv_list_FTP(CURL* curl, CURLcode& res, const std::string& base_url, std::string subfolder, const std::function<void(const GroupTask&)>& func, const TaskParams& dir);
    static bool redirect_curl(const SimpleTask& task);
    static bool file_download_curl(const UrlParams& from, const fs::path& source, const fs::path& destination);

};