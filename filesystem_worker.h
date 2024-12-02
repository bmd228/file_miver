#pragma once
#include <filesystem>
#include <spdlog/spdlog.h>
#include <re2/re2.h>
#include "utils.h"
#include "task_params.h"
#include "ftp_worker.h"
#include "s3_worker.h"

namespace fs = std::filesystem;
struct FilesystemWorker
{
	static bool scaner_dirs(const std::function<void(const GroupTask&)>& func,const TaskParams&);
	static bool fs_to_fs(const SimpleTask& task, const bool& only_move);
	static bool fs_to_ftp(const SimpleTask& task, const bool& only_move);
	static bool fs_to_s3(const SimpleTask& task, const bool& only_move);
	static bool remove(const fs::path& source);
	//static void FilesystemWorker::remove_empty_directory(const fs::path& dir);
};