#pragma once 
#include <filesystem>

#include <optional>
#include "utils.h"
namespace fs = std::filesystem;
enum FileMode
{
	COPY = 0,
	BALANCER = 1,
	ONLY_MOVE=3

};
enum Protocol
{
	DIR = 0,
	FTP = 1,
	S3 = 2,
};
enum StateController
{
	BLOCK = 0,
	UNBLOСK = 1
};

struct UrlParams
{
	UrlParams() = default;
	~UrlParams() = default;
	fs::path path;
	Protocol protocol;
	std::optional <std::string> mask;
	std::optional<std::string> entrypoint;
	std::optional<std::string> ftp_user_password;
	std::optional<std::string> access_key;
	std::optional<std::string> secret_key;
	std::optional<std::string> bucket;
	bool is_security = false;
	UrlParams(const fs::path& path,
		Protocol protocol = Protocol::DIR,
		std::optional<std::string> mask = std::nullopt,
		std::optional<std::string> entrypoint = std::nullopt,
		std::optional<std::string> ftp_user_password = std::nullopt,
		std::optional<std::string> access_key = std::nullopt,
		std::optional<std::string> secret_key = std::nullopt,
		std::optional<std::string> bucket = std::nullopt,
		bool is_security = false)
		: path(path), protocol(protocol), mask(mask), entrypoint(entrypoint),
		ftp_user_password(ftp_user_password), access_key(access_key),
		secret_key(secret_key), bucket(bucket), is_security(is_security) {}

};
struct TaskParams
{
	TaskParams() {
		id = generateRandom();
	}
	~TaskParams() = default;

	UrlParams from;
	std::vector<UrlParams> to;
	FileMode file_mode = FileMode::COPY;
	bool recursiv = false;
	size_t id{ 0 };
	void set_from(const fs::path& path,
		Protocol protocol = Protocol::DIR,
		std::optional<std::string> mask = std::nullopt,
		std::optional<std::string> entrypoint = std::nullopt,
		std::optional<std::string> ftp_user_password = std::nullopt,
		std::optional<std::string> access_key = std::nullopt,
		std::optional<std::string> secret_key = std::nullopt,
		std::optional<std::string> bucket = std::nullopt,
		bool is_security = false) {
		from= UrlParams(path, protocol, std::move(mask), std::move(entrypoint), std::move(ftp_user_password),
			std::move(access_key), std::move(secret_key), std::move(bucket), is_security);
		if(id==0)
			id = generateRandom();
	}
	void add_to(const fs::path& path,
		Protocol protocol = Protocol::DIR,
		std::optional<std::string> mask = std::nullopt,
		std::optional<std::string> entrypoint = std::nullopt,
		std::optional<std::string> ftp_user_password = std::nullopt,
		std::optional<std::string> access_key = std::nullopt,
		std::optional<std::string> secret_key = std::nullopt,
		std::optional<std::string> bucket = std::nullopt,
		bool is_security = false) {
		// Создаём новый UrlParams и добавляем его в вектор
		to.push_back(UrlParams(path, protocol, std::move(mask), std::move(entrypoint), std::move(ftp_user_password),
			std::move(access_key), std::move(secret_key), std::move(bucket), is_security));
	}
};
struct SimpleTask
{
	SimpleTask(const UrlParams& from, const UrlParams& to, const fs::path& relative_path, const fs::path& source, const size_t& id) :m_from(from), m_to(to), m_relative_path(relative_path), m_source(source),m_id(id) {};
	~SimpleTask() = default;
	UrlParams m_from;
	UrlParams m_to;
	fs::path m_relative_path;
	fs::path m_source;
	size_t m_id{ 0 };
};
struct GroupTask
{
public:
	GroupTask(const size_t& id) :m_id(id) {};
	~GroupTask() = default;
	void push_task(const UrlParams& from, const UrlParams& to, const fs::path& relative_path, const fs::path& source)
	{
		m_group_task.emplace_back(from,to, relative_path, source, m_id);
	};
	void set_file_mode(const FileMode& file_mode)
	{
		m_file_mode = file_mode;
	};

	size_t m_id{ 0 };
	std::vector<SimpleTask> m_group_task;
	FileMode m_file_mode = FileMode::COPY;

};