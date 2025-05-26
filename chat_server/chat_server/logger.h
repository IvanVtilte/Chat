#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <shared_mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

class Logger {
private:
    std::fstream log_file;
    mutable std::shared_mutex file_mutex;
    std::string get_current_time();

public:
    Logger(const std::string& filename);
    ~Logger();
    void write_log(const std::string& message);
    std::string read_log();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

#endif 
