#pragma once
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/attributes.hpp>
#include <boost/date_time.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <string>
#include <iostream>
#include <sstream>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", std::string)

// Форматтер для вывода в JSON
inline void JsonFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    std::ostringstream oss;
    oss << "{";
    
    // Добавляем timestamp
    if (auto ts = rec[timestamp]) {
        oss << "\"timestamp\":\"" << to_iso_extended_string(*ts) << "\",";
    }
    
    // Добавляем data
    if (auto data = rec[additional_data]) {
        oss << "\"data\":" << *data << ",";
    } else {
        oss << "\"data\":{},";
    }
    
    // Добавляем message
    if (auto msg = rec[logging::expressions::smessage]) {
        oss << "\"message\":\"" << *msg << "\"";
    }
    
    oss << "}";
    strm << oss.str();
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
