#pragma once

// RIGSOFRODS: Brought from rorserver's 'logger.h'

enum ServerLogLevel {
    LOG_STACK = 0,
    LOG_DEBUG,
    LOG_VERBOSE,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_NONE
};

namespace Logger {

    void Log(ServerLogLevel level, const char *format, ...);

    void Log(ServerLogLevel level, std::string const& msg);
}

