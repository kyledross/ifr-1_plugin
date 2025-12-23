#pragma once
#include "XPlaneSDK.h"
#include <format>
#include <string>

/**
 * @brief Logging macros that check log level before evaluating arguments and building the string.
 * Uses std::format for efficient formatting.
 */

#define IFR1_LOG(sdk, level, fmt_str, ...) \
    do { \
        if ((level) <= (sdk).GetLogLevel()) { \
            (sdk).Log((level), std::format(fmt_str __VA_OPT__(,) __VA_ARGS__).c_str()); \
        } \
    } while (0)

#define IFR1_LOG_ERROR(sdk, fmt_str, ...) IFR1_LOG(sdk, LogLevel::Error, fmt_str __VA_OPT__(,) __VA_ARGS__)
#define IFR1_LOG_INFO(sdk, fmt_str, ...) IFR1_LOG(sdk, LogLevel::Info, fmt_str __VA_OPT__(,) __VA_ARGS__)
#define IFR1_LOG_VERBOSE(sdk, fmt_str, ...) IFR1_LOG(sdk, LogLevel::Verbose, fmt_str __VA_OPT__(,) __VA_ARGS__)

// For conditional logging with an extra condition
#define IFR1_LOG_VERBOSE_IF(sdk, cond, fmt_str, ...) \
    do { \
        if ((cond) && (LogLevel::Verbose <= (sdk).GetLogLevel())) { \
            (sdk).Log(LogLevel::Verbose, std::format(fmt_str __VA_OPT__(,) __VA_ARGS__).c_str()); \
        } \
    } while (0)
