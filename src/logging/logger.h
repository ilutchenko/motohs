#pragma once

#include <string>
#include <array>
#include <stdio.h>
#include <time.h>

#include <fmt/core.h>
#include <fmt/chrono.h>

using string_view_t = fmt::basic_string_view<char>;
using wstring_view_t = fmt::basic_string_view<wchar_t>;

// Foreground colors
const string_view_t black = "\033[30m";
const string_view_t red = "\033[31m";
const string_view_t green = "\033[32m";
const string_view_t yellow = "\033[33m";
const string_view_t blue = "\033[34m";
const string_view_t magenta = "\033[35m";
const string_view_t cyan = "\033[36m";
const string_view_t white = "\033[37m";
/// Bold colors
const string_view_t yellow_bold = "\033[33m\033[1m";
const string_view_t red_bold = "\033[31m\033[1m";
const string_view_t bold_on_red = "\033[1m\033[41m";

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

    static bool init(bool color_mode = true)
    {
        colors_[Logger::debug] = to_string_(cyan);
        colors_[Logger::info] = to_string_(green);
        colors_[Logger::warn] = to_string_(yellow_bold);
        colors_[Logger::err] = to_string_(red_bold);
        set_color_mode(color_mode);
        return true;
    }

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

    static void set_color_mode(bool mode)
    {
        should_do_colors_ = mode;
    }

private:
    // common implementation for after templated public api has been resolved
    template<typename FormatString, typename... Args>
    static void log_(const char *tag, Logger::level_enum lvl, const FormatString &fmt,
                     const Args &...args)
    {
        try {
            auto s = colors_[lvl] + prefix(tag) + fmt::format(fmt, args...);
            printf("%s\n", s.data());
        } catch (...) {
            printf("log error\n");
        }
    }

    static std::string prefix(const char *tag)
    {
        time_t now;
        struct tm t;

        time(&now);
        localtime_r(&now, &t);

        return fmt::format("[{:%H:%M:%S}][{:^10}]:{} ", t, tag, white);
    }

    inline static std::string to_string_(const string_view_t &sv)
    {
        return std::string(sv.data(), sv.size());
    }

    inline static bool should_do_colors_{true};
    inline static std::array<std::string, Logger::n_levels> colors_;
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
