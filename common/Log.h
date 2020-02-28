#pragma once
#include <stdio.h>
#include <string.h>
#include <string>

enum LogPriority {
	LOG_PRIORITY_ERROR = 0,
	LOG_PRIORITY_INFO,
	LOG_PRIORITY_DEBUG,
	LOG_PRIORITY_TRACE,

	LOG_PRIORITY_MAX,
};

class Log {
protected:
	Log();

public:
	static Log& Instance() {
		static Log logInstance;
		return logInstance;
	}

	~Log();

	LogPriority Priority();
	void SetLogPriority(LogPriority pri);
	void SetLogDir(const char* dir);
	void SetLogSize(long size);

	void Write(LogPriority pri, const char* file, int line, const char* func, const char* fmt, ...);
	void Write(const char* data);

	void Flush();
	void Close();

private:
	FILE* GetFile(LogPriority pri);
	void FileRoll(LogPriority pri);

private:
	LogPriority		logPri_;
	std::string		logDir_;
	long			maxSize_;
	FILE*			files_[LOG_PRIORITY_MAX];
};

#define PLOG_ERROR(fmt, ...) \
	do { \
		if (Log::Instance().Priority() >= LOG_PRIORITY_ERROR) { \
			Log::Instance().Write(LOG_PRIORITY_ERROR, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); \
		} \
	} while (0)
#define PLOG_INFO(fmt, ...) \
	do { \
		if (Log::Instance().Priority() >= LOG_PRIORITY_INFO) { \
			Log::Instance().Write(LOG_PRIORITY_INFO, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); \
		} \
	} while (0)
#define PLOG_DEBUG(fmt, ...) \
	do { \
		if (Log::Instance().Priority() >= LOG_PRIORITY_DEBUG) { \
			Log::Instance().Write(LOG_PRIORITY_DEBUG, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); \
		} \
	} while (0)
#define PLOG_TRACE(fmt, ...) \
	do { \
		if (Log::Instance().Priority() >= LOG_PRIORITY_TRACE) { \
			Log::Instance().Write(LOG_PRIORITY_TRACE, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); \
		} \
	} while (0)
