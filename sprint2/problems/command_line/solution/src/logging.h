#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

#include <boost/date_time.hpp>
#include <boost/json.hpp>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace json = boost::json;


BOOST_LOG_ATTRIBUTE_KEYWORD(
    timestamp,
    "TimeStamp",
    boost::posix_time::ptime
)

BOOST_LOG_ATTRIBUTE_KEYWORD(
    additional_data,
    "AdditionalData",
    json::value
)


inline void JsonFormatter(
    logging::record_view const& rec,
    logging::formatting_ostream& strm
) {
    json::object result;

    if (auto ts = rec[timestamp]) {
        result["timestamp"] =
            boost::posix_time::to_iso_extended_string(*ts);
    }

    if (auto data = rec[additional_data]) {
        result["data"] = *data;
    } else {
        result["data"] = json::object{};
    }

    if (auto msg = rec[logging::expressions::smessage]) {
        result["message"] = *msg;
    }

    strm << json::serialize(result);
}


inline void InitLogging() {
    logging::add_common_attributes();

    logging::add_console_log(
        std::clog,
        keywords::format = &JsonFormatter,
        keywords::auto_flush = true
    );

    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::info
    );
}
