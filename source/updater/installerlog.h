#ifndef INSTALLERLOG_H__
#define INSTALLERLOG_H__

#include <time.h>

#include <wx/thread.h>
#include <wx/event.h>

#include "boost/asio.hpp"
#include "boost/filesystem.hpp"

#define LOG(...) do { InstallerLog::getSingleton()->logMessageNoNewLine(__VA_ARGS__); } while(0)
#define LOGN(...) do { InstallerLog::getSingleton()->logMessage(__VA_ARGS__); } while(0)


class InstallerLog
{
public:
	InstallerLog(boost::filesystem::path logFilename);
	~InstallerLog();
	static InstallerLog *getSingleton();

	void logMessage(const char* fmt, ...);
	void logMessageNoNewLine(const char* fmt, ...);

	FILE *getLogFilePtr() { return file; };

	static InstallerLog *instance;
protected:
	wxMutex logmutex;
	FILE *file;
};

#endif //INSTALLERLOG_H__
