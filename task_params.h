#pragma once 
#include <filesystem>

#include <optional>

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
	std::string mask;
	std::optional<std::string> entrypoint;
	std::optional<std::string> ftp_user_password;
	std::optional<std::string> access_key;
	std::optional<std::string> secret_key;
	std::optional<std::string> bucket;
	bool is_security = false;
	UrlParams(const fs::path& path,
		Protocol protocol = Protocol::DIR,
		const std::string_view mask = "",
		std::optional<std::string_view> entrypoint = std::nullopt,
		std::optional<std::string_view> ftp_user_password = std::nullopt,
		std::optional<std::string_view> access_key = std::nullopt,
		std::optional<std::string_view> secret_key = std::nullopt,
		std::optional<std::string_view> bucket = std::nullopt,
		bool is_security = false)
		: path(path), protocol(protocol), mask(mask), entrypoint(entrypoint),
		ftp_user_password(ftp_user_password), access_key(access_key),
		secret_key(secret_key), bucket(bucket), is_security(is_security) {}

};
struct TaskParams
{
	TaskParams() = default;
	~TaskParams() = default;

	UrlParams from;
	std::vector<UrlParams> to;
	FileMode file_mode = FileMode::COPY;
	bool recursiv = false;
	void set_from(const fs::path& path,
		Protocol protocol = Protocol::DIR,
		const std::string& mask = "",
		std::optional<std::string> entrypoint = std::nullopt,
		std::optional<std::string> ftp_user_password = std::nullopt,
		std::optional<std::string> access_key = std::nullopt,
		std::optional<std::string> secret_key = std::nullopt,
		std::optional<std::string> bucket = std::nullopt,
		bool is_security = false) {
		from= UrlParams(path, protocol, mask, std::move(entrypoint), std::move(ftp_user_password),
			std::move(access_key), std::move(secret_key), std::move(bucket), is_security);
	}
	void add_to(const fs::path& path,
		Protocol protocol = Protocol::DIR,
		const std::string& mask = "",
		std::optional<std::string> entrypoint = std::nullopt,
		std::optional<std::string> ftp_user_password = std::nullopt,
		std::optional<std::string> access_key = std::nullopt,
		std::optional<std::string> secret_key = std::nullopt,
		std::optional<std::string> bucket = std::nullopt,
		bool is_security = false) {
		// Создаём новый UrlParams и добавляем его в вектор
		to.push_back(UrlParams(path, protocol, mask, std::move(entrypoint), std::move(ftp_user_password),
			std::move(access_key), std::move(secret_key), std::move(bucket), is_security));
	}
};
struct SimpleTask
{
	SimpleTask(const UrlParams& from, const UrlParams& to, const fs::path& relative_path, const fs::path& source) :m_from(from), m_to(to), m_relative_path(relative_path), m_source(source) {};
	~SimpleTask() = default;
	UrlParams m_from;
	UrlParams m_to;
	fs::path m_relative_path;
	fs::path m_source;
};
struct GroupTask
{
public:
	void push_task(const UrlParams& from, const UrlParams& to, const fs::path& relative_path, const fs::path& source)
	{
		m_group_task.emplace_back(from,to, relative_path, source);
	};
	void set_file_mode(const FileMode& file_mode)
	{
		m_file_mode = file_mode;
	};


	std::vector<SimpleTask> m_group_task;
	FileMode m_file_mode = FileMode::COPY;

};