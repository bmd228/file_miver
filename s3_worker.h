#pragma once
#if WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN  // исключаем ненужные заголовки
#include <windows.h>
#endif()
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/Bucket.h>
#include <aws/s3/model/CreateBucketConfiguration.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/GetObjectAclResult.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/DeleteObjectResult.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/ListObjectsResult.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/ListObjectsV2Result.h>
#include <aws/s3/model/ListDirectoryBucketsRequest.h>
#include <aws/s3/model/ListDirectoryBucketsResult.h>
#include <spdlog/spdlog.h>
#include <re2/re2.h>
#include <fstream>
#include "task_params.h"
#include "ftp_worker.h"
struct S3Worker
{
	static bool scaner_S3(const std::function<void(const GroupTask&)>& func, const TaskParams&);
	static bool s3_to_fs(const SimpleTask& task, const bool& only_move);
	static bool s3_to_ftp(const SimpleTask& task, const bool& only_move);
	static bool s3_to_s3(const SimpleTask& task, const bool& only_move);
	static bool file_download(const UrlParams& from, const fs::path& source, const fs::path& destination);
	static bool file_upload(const UrlParams& to, const fs::path& source, const fs::path& destination);
	static bool remove(const SimpleTask& task);
};