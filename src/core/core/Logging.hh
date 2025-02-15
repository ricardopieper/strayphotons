#pragma once

#include "core/Common.hh"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <magic_enum.hpp>
#include <memory>
#include <string>
#include <tracy/Tracy.hpp>
#include <type_traits>

#define Tracef(...) ::sp::logging::Trace(__FILE__, __LINE__, __VA_ARGS__)
#define Debugf(...) ::sp::logging::Debug(__FILE__, __LINE__, __VA_ARGS__)
#define Logf(...) ::sp::logging::Log(__FILE__, __LINE__, __VA_ARGS__)
#define Warnf(...) ::sp::logging::Warn(__FILE__, __LINE__, __VA_ARGS__)
#define Errorf(...) ::sp::logging::Error(__FILE__, __LINE__, __VA_ARGS__)
#define Abortf(...) ::sp::logging::Abort(__FILE__, __LINE__, __VA_ARGS__)
#define Assertf(condition, ...) \
    if (!(condition)) ::sp::logging::Abort(__FILE__, __LINE__, __VA_ARGS__)

namespace sp::logging {
    enum class Level { Error, Warn, Log, Debug, Trace };

    // time in seconds
    float LogTime();
    Level GetLogLevel();
    void SetLogLevel(Level level);

    void GlobalLogOutput(Level lvl, const std::string &line);

    inline static const char *basename(const char *file) {
        const char *r;
        if ((r = strrchr(file, '/'))) return r + 1;
        if ((r = strrchr(file, '\\'))) return r + 1;
        return file;
    }

    // Convert all std::strings to const char* using constexpr if (C++17)
    // Source: https://gist.github.com/Zitrax/a2e0040d301bf4b8ef8101c0b1e3f1d5
    // Modified to support string_view and enums via magic_enum.hpp
    template<typename T>
    auto convert(T &&t) {
        using BaseType = std::remove_cv_t<std::remove_reference_t<T>>;

        if constexpr (std::is_same<BaseType, std::string>()) {
            return std::forward<T>(t).c_str();
        } else if constexpr (std::is_same<BaseType, std::string_view>()) {
            if (t.empty()) return "";
            Assert(t.data()[t.size()] == '\0', "string_view is not null terminated");
            return std::forward<T>(t).data();
        } else if constexpr (std::is_enum_v<BaseType>) {
            if (magic_enum::enum_name(std::forward<T>(t)).empty()) return "invalid_enum";
            return magic_enum::enum_name(std::forward<T>(t)).data();
        } else {
            return std::forward<T>(t);
        }
    }

    template<typename... T>
    inline static void writeFormatter(Level lvl, const std::string &fmt, T &&...t) {
        int size = std::snprintf(nullptr, 0, fmt.c_str(), std::forward<T>(t)...);
        std::unique_ptr<char[]> buf(new char[size + 1]);
        std::snprintf(buf.get(), size + 1, fmt.c_str(), std::forward<T>(t)...);

        TracyMessage(buf.get(), size);
        if (lvl > GetLogLevel()) return;
        std::cerr << buf.get();
        if (lvl < Level::Debug) {
            GlobalLogOutput(lvl, string(buf.get(), buf.get() + size));
        }
    }

    template<typename... Tn>
    inline static void writeLog(Level lvl, const char *file, int line, const std::string &fmt, Tn &&...tn) {
        writeFormatter(lvl, "%.3f " + fmt + "\n", LogTime(), convert(std::forward<Tn>(tn))...);
    }

    template<typename... T>
    static void ConsoleWrite(Level lvl, const std::string &fmt, T... t) {
        writeFormatter(lvl, fmt + "\n", convert(std::forward<T>(t))...);
    }

    template<typename... T>
    static void Trace(const char *file, int line, const std::string &fmt, T... t) {
        writeLog(Level::Trace, file, line, "[trace] " + fmt, t...);
    }

    template<typename... T>
    static void Debug(const char *file, int line, const std::string &fmt, T... t) {
        writeLog(Level::Debug, file, line, "[dbg] " + fmt, t...);
    }

    template<typename... T>
    static void Log(const char *file, int line, const std::string &fmt, T... t) {
        writeLog(Level::Log, file, line, "[log] " + fmt, t...);
    }

    template<typename... T>
    static void Warn(const char *file, int line, const std::string &fmt, T... t) {
        writeLog(Level::Warn, file, line, "[warn] " + fmt, t...);
    }

    template<typename... T>
    static void Error(const char *file, int line, const std::string &fmt, T... t) {
        writeLog(Level::Error, file, line, "[error] " + fmt, t...);
    }

    template<typename... T>
    [[noreturn]] static void Abort(const char *file, int line, const std::string &fmt, T... t) {
        writeLog(Level::Error, file, line, "[abort] " + fmt, t...);
        sp::Abort();
    }
} // namespace sp::logging

namespace sp {
    struct LogOnExit {
        const char *message;
        LogOnExit(const char *message) : message(message) {}
        ~LogOnExit() {
            if (logging::Level::Debug > logging::GetLogLevel()) return;
            std::cout << std::fixed << std::setprecision(3) << logging::LogTime() << " [log] " << message << std::endl
                      << std::flush;
        }
    };
} // namespace sp
