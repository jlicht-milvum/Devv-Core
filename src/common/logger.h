#ifndef DEVCASH_LOGGER_H
#define DEVCASH_LOGGER_H

#define BOOST_LOG_DYN_LINK \
  1  // necessary when linking the boost_log library dynamically

#include <boost/log/trivial.hpp>
#include <boost/log/sources/global_logger_storage.hpp>

// the logs are also written to LOGFILE
#define LOGFILE "logfile.log"

// just log messages with severity >= SEVERITY_THRESHOLD are written
#define SEVERITY_THRESHOLD logging::trivial::warning

//Narrow-char thread-safe logger.
typedef boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level> logger_t;

// register a global logger
BOOST_LOG_GLOBAL_LOGGER(logger, logger_t)

// just a helper macro used by the macros below - don't use it in your code
#define LOG(severity) \
  BOOST_LOG_SEV(logger::get(), boost::log::trivial::severity)

// ===== log macros =====
#define LOG_TRACE LOG(trace)
#define LOG_DEBUG LOG(debug)
#define LOG_INFO LOG(info)
#define LOG_WARNING LOG(warning)
#define LOG_ERROR LOG(error)
#define LOG_FATAL LOG(fatal)

#endif  // DEVCASH_LOGGER_H
