#include "Logger.h"
#include <stdexcept>

std::string Logger::get_current_time() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}

Logger::Logger(const std::string& filename) {
    log_file.open(filename, std::ios::in | std::ios::out | std::ios::app);
    if (!log_file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл логов");
    }
}

Logger::~Logger() {
    std::unique_lock<std::shared_mutex> lock(file_mutex);
    if (log_file.is_open()) {
        log_file.close();
    }
}

void Logger::write_log(const std::string& message) {
    std::unique_lock<std::shared_mutex> lock(file_mutex);
    if (log_file.is_open()) {
        log_file << "[" << get_current_time() << "] " << message << std::endl;
    }
}

std::string Logger::read_log() {
    std::string line;
    std::shared_lock<std::shared_mutex> lock(file_mutex);
    if (log_file.is_open()) {
        log_file.seekg(0, std::ios::beg);
        std::getline(log_file, line);
    }
    return line;
}
