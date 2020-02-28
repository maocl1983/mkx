#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#include <vector>
#include <algorithm>
#include "Log.h"

const int LOG_BUFF_LEN = 4096;
static const char* priorityStr[] = {
	"ERROR", "INFO", "DEBUG", "TRACE"	
};
static const char* filePrefix[] = {
	"error", "debug", "debug", "debug"
};


//================================================================================
Log::Log()
{
	for (int i = 0; i < LOG_PRIORITY_MAX; i++) {
		files_[i] = nullptr;
	}
	logPri_ = LOG_PRIORITY_TRACE;
	maxSize_ = 1024 * 1024 * 1024;
}

Log::~Log()
{
	Flush();
	Close();
}

LogPriority Log::Priority()
{
	return logPri_;
}

void Log::SetLogPriority(LogPriority pri)
{
	logPri_ = pri;
}

void Log::SetLogDir(const char* dir)
{
	logDir_ = dir;
}

void Log::SetLogSize(long size)
{
	if (size > 0) {
		maxSize_ = size;
	}
}

void Log::Write(LogPriority pri, const char* file, int line, const char* func, const char* fmt, ...)
{
	if (pri > logPri_) {
		return;
	}

	static char buff[LOG_BUFF_LEN] = {0};

	int len = 0;
	time_t nowtime;
	time(&nowtime);
	len = strftime(buff, LOG_BUFF_LEN, "[%Y-%m-%d %H:%M:%S]", localtime(&nowtime));

	len += snprintf(buff + len, LOG_BUFF_LEN - len, "[%d][%s:%d][%s][%s] ",
			getpid(), file, line, func, priorityStr[pri]);

	va_list ap;
	va_start(ap, fmt);
	len += vsnprintf(buff + len, LOG_BUFF_LEN - len, fmt, ap);
	va_end(ap);

	if (len >= LOG_BUFF_LEN - 2) {
		len = LOG_BUFF_LEN - 2;
	}
	buff[len++] = '\n';
	buff[len] = '\0';
	
	FILE* logfile = GetFile(pri);
	if (logfile) {
		fwrite(buff, len, 1, logfile);
		fflush(logfile);
	}
}

void Log::Write(const char* data)
{
	/*FILE* logfile = GetFile();
	if (logfile) {
		fwrite(data, strlen(data), 1, logfile);
	}*/
}

void Log::Flush()
{
	for (int i = 0; i < LOG_PRIORITY_MAX; i++) {
		if (files_[i]) {
			fflush(files_[i]);
		}
	}
}

void Log::Close()
{
	for (int i = 0; i < LOG_PRIORITY_MAX; i++) {
		if (files_[i]) {
			fclose(files_[i]);
			files_[i] = nullptr;
		}
	}
}

FILE* Log::GetFile(LogPriority pri)
{
	if (!files_[pri]) {
		static char filename[128] = {0};
		snprintf(filename, sizeof(filename)-1, "%s/%s.log", 
				logDir_.c_str(), filePrefix[pri]);
		files_[pri] = fopen(filename, "a+");
	} else {
		long size = ftell(files_[pri]);
		if (size < 0 || size > maxSize_) {
			FileRoll(pri);
		}
	}

	return files_[pri];
}

void Log::FileRoll(LogPriority pri)
{
	if (files_[pri]) {
		fclose(files_[pri]);
		files_[pri] = nullptr;
	}

	std::string logName = logDir_ + "/" + filePrefix[pri] + ".log";
	std::string match = logName + ".*";

	DIR* logdir;
	struct dirent* fptr;
	std::vector<int> fileIdx;
	if ((logdir = opendir(logDir_.c_str())) == NULL) {
		return;
	}
	while ((fptr = readdir(logdir)) != NULL) {
		if (strcmp(fptr->d_name,".")==0 || strcmp(fptr->d_name,"..")==0) {
			continue;
		}
		if (fptr->d_type == 8) {
			std::string namestr(fptr->d_name);
			std::string::size_type pos = namestr.find_last_of('.');
			if (pos != std::string::npos) {
				int idx = atoi(namestr.substr(pos + 1).c_str());
				if (idx > 0) {
					fileIdx.push_back(idx);
				}
			}
		}
	}

	std::sort(fileIdx.begin(), fileIdx.end(), std::greater<int>());
	for (size_t i = 0; i < fileIdx.size(); i++) {
		int idx = fileIdx[i];
		std::string oldname = logName + "." + std::to_string(idx);
		std::string newname = logName + "." + std::to_string(idx+1);
		rename(oldname.c_str(), newname.c_str());
	}

	std::string newname = logName + ".1";
	rename(logName.c_str(), newname.c_str());

	files_[pri] = fopen(logName.c_str(), "w+");
}

