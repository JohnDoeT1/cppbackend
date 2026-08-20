#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>
#include <iostream>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    auto GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }
        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

    std::string GetFileTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&t_c), "%Y_%m_%d");
        return oss.str();
    }

    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    template<class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Получаем текущую дату для имени файла
        const auto file_date = GetFileTimeStamp();
        const std::string filename = "/var/log/sample_log_" + file_date + ".log";
        
        // Если файл изменился (смена даты), открываем новый
        if (filename != current_filename_) {
            if (log_file_.is_open()) {
                log_file_.close();
            }
            log_file_.open(filename, std::ios::app);
            if (!log_file_.is_open()) {
                // В случае ошибки открытия файла, выводим в stderr
                std::cerr << "Failed to open log file: " << filename << std::endl;
                return;
            }
            current_filename_ = filename;
        }
        
        // Выводим временную метку
        log_file_ << GetTimeStamp() << ": ";
        
        // Выводим все аргументы через fold expression
        ((log_file_ << args), ...);
        
        log_file_ << std::endl;
    }

    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard<std::mutex> lock(mutex_);
        manual_ts_ = ts;
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::mutex mutex_;
    std::ofstream log_file_;
    std::string current_filename_;
};
