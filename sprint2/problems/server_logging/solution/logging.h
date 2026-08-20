#pragma once
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/attributes.hpp>
#include <boost/json.hpp>
#include <boost/date_time.hpp>
#include <string>
#include <iostream>

namespace json = boost::json;
namespace logging = boost::log;
namespace keywords = boost::log::keywords;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

// Форматтер для вывода в JSON
inline void JsonFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    json::object log_entry;
    
    // Добавляем timestamp
    if (auto ts = rec[timestamp]) {
        log_entry["timestamp"] = json::string(to_iso_extended_string(*ts));
    }
    
    // Добавляем data
    if (auto data = rec[additional_data]) {
        log_entry["data"] = *data;
    } else {
        log_entry["data"] = json::object{};
    }
    
    // Добавляем message
    if (auto msg = rec[logging::expressions::smessage]) {
        log_entry["message"] = json::string(*msg);
    }
    
    strm << json::serialize(log_entry);
}

// Инициализация логирования
inline void InitLogging() {
    logging::add_common_attributes();
    
    logging::add_console_log(
        std::cout,
        keywords::format = &JsonFormatter,
        keywords::auto_flush = true
    );
    
    // Устанавливаем фильтр для вывода info и выше
    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::info
    );
}
