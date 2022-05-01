#pragma once

#include <string>
#include <fmt/core.h>

using string_view_t = fmt::basic_string_view<char>;
using wstring_view_t = fmt::basic_string_view<wchar_t>;

class Logger
{
public:
    enum level_enum : int
    {
        debug = 0,
        info,
        warn,
        err,
        n_levels
    };

    // FormatString is a type derived from fmt::compile_string
    template<typename FormatString,
             typename std::enable_if<fmt::is_compile_string<FormatString>::value,
                                     int>::type = 0,
             typename... Args>
    static void log(const char *tag, Logger::level_enum lvl, const FormatString &fmt,
                    const Args &...args)
    {
        log_(tag, lvl, fmt, args...);
    }

    // FormatString is NOT a type derived from fmt::compile_string but is a string_view_t
    // or can be implicitly converted to one
    template<typename... Args>
    static void log(const char *tag, Logger::level_enum lvl, string_view_t fmt,
                    const Args &...args)
    {
        log_(tag, lvl, fmt, args...);
    }

    template<typename FormatString, typename... Args>
    static void log(Logger::level_enum lvl, const char *tag, const FormatString &fmt,
                    const Args &...args)
    {
        log(tag, lvl, fmt, args...);
    }

    template<typename FormatString, typename... Args>
    static void log_debug(const char *tag, const FormatString &fmt, const Args &...args)
    {
        log(Logger::debug, tag, fmt, args...);
    }

    template<typename FormatString, typename... Args>
    static void log_info(const char *tag, const FormatString &fmt, const Args &...args)
    {
        log(Logger::info, tag, fmt, args...);
    }

    template<typename FormatString, typename... Args>
    static void log_warn(const char *tag, const FormatString &fmt, const Args &...args)
    {
        log(Logger::warn, tag, fmt, args...);
    }

    template<typename FormatString, typename... Args>
    static void log_error(const char *tag, const FormatString &fmt, const Args &...args)
    {
        log(Logger::err, tag, fmt, args...);
    }

private:
    // common implementation for after templated public api has been resolved
    template<typename FormatString, typename... Args>
    static void log_(const char *tag, Logger::level_enum lvl, const FormatString &fmt,
                     const Args &...args)
    {
        try {
            auto s = fmt::format(fmt, args...);
            printf("%s\n", s.data());
        } catch (...) {
            printf("log error\n");
        }
    }
};

#define LOGI(...) Logger::log_info(TAG, __VA_ARGS__)
#define LOGW(...) Logger::log_warn(TAG, __VA_ARGS__)
#define LOGE(...) Logger::log_error(TAG, __VA_ARGS__)

#if BUILD_TYPE_RELEASE == 1
#define LOGD(...) static_cast<void>(0)
#else
#define LOGD(...) Logger::log_debug(TAG, __VA_ARGS__)
#endif

#define LOGI_TAGGED(tag, ...) Logger::log_info(tag, __VA_ARGS__)
#define LOGW_TAGGED(tag, ...) Logger::log_warn(tag, __VA_ARGS__)
#define LOGE_TAGGED(tag, ...) Logger::log_error(tag, __VA_ARGS__)

#if BUILD_TYPE_RELEASE == 1
#define LOGD_TAGGED(tag, ...) static_cast<void>(0)
#else
#define LOGD_TAGGED(tag, ...) Logger::log_debug(tag, __VA_ARGS__)
#endif
