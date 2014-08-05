#include "installerlog.h"

InstallerLog *InstallerLog::instance = 0;

InstallerLog::InstallerLog(boost::filesystem::path logFilename) : file(0)
{
	InstallerLog::instance = this;
	this->file = fopen(logFilename.string().c_str(), "w");
}

InstallerLog::~InstallerLog()
{
}

InstallerLog *InstallerLog::getSingleton()
{
	return InstallerLog::instance;
}

void InstallerLog::logMessage(const char* fmt, ...)
{
	if(!this->file) return;
	wxMutexLocker lock(logmutex);

	va_list args;
	va_start(args, fmt);
	vfprintf(this->file, fmt, args);
	fprintf(this->file, "\n");
	fflush(this->file);
	va_end(args);
}

void InstallerLog::logMessageNoNewLine(const char* fmt, ...)
{
	if(!this->file) return;
	wxMutexLocker lock(logmutex);

	va_list args;
	va_start(args, fmt);
	vfprintf(this->file, fmt, args);
	fflush(this->file);
	va_end(args);
}