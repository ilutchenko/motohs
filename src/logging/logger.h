#pragma once

#include <string>
#include <array>
#include <stdio.h>
#include <time.h>
#include <sstream>

#include <fmt/core.h>
#include <fmt/chrono.h>
#include "esp_log.h"

using string_view_t = fmt::basic_string_view<char>;
using wstring_view_t = fmt::basic_string_view<wchar_t>;

// Formatting codes
const std::string reset = "\033[m";
const std::string bold = "\033[1m";
const std::string dark = "\033[2m";
const std::string underline = "\033[4m";
const std::string blink = "\033[5m";
const std::string reverse = "\033[7m";
const std::string concealed = "\033[8m";
const std::string clear_line = "\033[K";

// Foreground colors
const std::string black = "\033[30m";
const std::string red = "\033[31m";
const std::string green = "\033[32m";
const std::string yellow = "\033[33m";
const std::string blue = "\033[34m";
const std::string magenta = "\033[35m";
const std::string cyan = "\033[36m";
const std::string white = "\033[37m";

/// Background colors
const std::string on_black = "\033[40m";
const std::string on_red = "\033[41m";
const std::string on_green = "\033[42m";
const std::string on_yellow = "\033[43m";
const std::string on_blue = "\033[44m";
const std::string on_magenta = "\033[45m";
const std::string on_cyan = "\033[46m";
const std::string on_white = "\033[47m";

/// Bold colors
const std::string yellow_bold = "\033[33m\033[1m";
const std::string red_bold = "\033[31m\033[1m";
const std::string bold_on_red = "\033[1m\033[41m";

static constexpr auto TAIL_ESCAPE_SEQ_LEN = 5;
static constexpr auto MESSAGE_PRINT_BUFFER = 256;

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
        colors_[Logger::warn] = to_string_(yellow);
        colors_[Logger::err] = to_string_(red);
        set_color_mode(color_mode);
        esp_log_set_vprintf(&Logger::vprintf_override);
        return true;
    }

    // FormatString is a type derived from fmt::compile_string
    template<typename FormatString,
             typename std::enable_if<fmt::is_compile_string<FormatString>::value,
                                     int>::type = 0,
             typename... Args>
    static void log(std::string tag, Logger::level_enum lvl, const FormatString &fmt,
                    const Args &...args)
    {
        log_(tag, lvl, fmt, args...);
    }

    // FormatString is NOT a type derived from fmt::compile_string but is a string_view_t
    // or can be implicitly converted to one
    template<typename... Args>
    static void log(std::string tag, Logger::level_enum lvl, string_view_t fmt,
                    const Args &...args)
    {
        log_(tag, lvl, fmt, args...);
    }

    template<typename FormatString, typename... Args>
    static void log(Logger::level_enum lvl, std::string tag, const FormatString &fmt,
                    const Args &...args)
    {
        log(tag, lvl, fmt, args...);
    }

    template<typename FormatString, typename... Args>
    static void log_debug(std::string tag, const FormatString &fmt, const Args &...args)
    {
        log(Logger::debug, tag, fmt, args...);
    }

    template<typename FormatString, typename... Args>
    static void log_info(std::string tag, const FormatString &fmt, const Args &...args)
    {
        log(Logger::info, tag, fmt, args...);
    }

    template<typename FormatString, typename... Args>
    static void log_warn(std::string tag, const FormatString &fmt, const Args &...args)
    {
        log(Logger::warn, tag, fmt, args...);
    }

    template<typename FormatString, typename... Args>
    static void log_error(std::string tag, const FormatString &fmt, const Args &...args)
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
    static void log_(std::string tag, Logger::level_enum lvl, const FormatString &fmt,
                     const Args &...args)
    {
        try {
            auto s = colors_[lvl] + prefix(tag) + fmt::format(fmt, args...);
            printf("%s\n", s.data());
        } catch (...) {
            printf("log error\n");
        }
    }

    static void log_(std::string tag, Logger::level_enum lvl, std::string msg)
    {
        try {
            auto s = colors_[lvl] + prefix(tag) + msg + reset;
            printf("%s\n", s.c_str());
        } catch (...) {
            printf("log error\n");
        }
    }

    static std::string prefix(std::string tag)
    {
        time_t now;
        struct tm t;

        time(&now);
        localtime_r(&now, &t);

        return fmt::format("{}[{:%H:%M:%S}][{:^10}]:{} ", bold, t, tag, white);
    }

    inline static std::string to_string_(const string_view_t &sv)
    {
        return std::string(sv.data(), sv.size());
    }

    inline static bool should_do_colors_{true};
    inline static std::array<std::string, Logger::n_levels> colors_;
    static void parse_message(std::string msg)
    {
        size_t begin;

        Logger::level_enum level;
        std::string tag;
        std::string log_msg;

        // Exclude terminal color symbols at the beginning of string
        if ((begin = msg.find("I (")) != std::string::npos) {
            level = Logger::info;
        } else if ((begin = msg.find("W (")) != std::string::npos) {
            level = Logger::warn;
        } else if ((begin = msg.find("E (")) != std::string::npos) {
            level = Logger::err;
        } else if ((begin = msg.find("D (")) != std::string::npos) {
            level = Logger::debug;
        } else {
            return;
        }

        // Strip text between start and end escape sequences.
        std::string sub = msg.substr(begin, (msg.size() - begin - TAIL_ESCAPE_SEQ_LEN));
        sub += '\n';
        std::stringstream iss(sub);
        iss.ignore(std::numeric_limits<std::streamsize>::max(), ')');
        iss.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
        std::getline(iss, tag, ':');
        iss.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
        std::getline(iss, log_msg);
        log_(tag, level, log_msg);
        return;
    }

    static int vprintf_override(const char *fmt, va_list args)
    {
        char buf[MESSAGE_PRINT_BUFFER];
        size_t size = vsnprintf(buf, MESSAGE_PRINT_BUFFER, fmt, args);
        parse_message(std::string{buf});
        return size;
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
