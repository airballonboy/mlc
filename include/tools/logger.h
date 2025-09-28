#ifndef MASTERLOGGER
#define MASTERLOGGER

#include <print>
#include "tools/format.h"

namespace mlog {


enum color{
    NoColor = 0,
    Black   = 30,
    Red     = 31,
    Green   = 32,
    Yellow  = 33,
    Blue    = 34,
    Magenta = 35,
    Cyan    = 36,
    White   = 37,
};

inline void log(const color color, const char* text)
{
    std::println("\033[{}m{}\033[0m", (int)color, text);
}
inline void log(const char* prefix, const color color, const char* text)
{
    std::println("\033[36m{}\033[{}m{}\033[0m", prefix, (int)color, text);
}
inline void log(const color color_prefix, const char* prefix, const color color, const char* text)
{
    std::println("\033[{}m{}\033[{}m{}\033[0m", (int)color_prefix, prefix, (int)color, text);
}

inline void success(const char* text)
{
    std::println("\033[32m{}\033[0m", text);
}
inline void success(const char* prefix, const char* successMassage)
{
    std::println("\033[36m{}\033[32m{}\033[0m", prefix, successMassage);
}


inline void error(const char* error)
{
    std::println("\033[31m{}\033[0m", error);
}
inline void error(const char* prefix, const char* errorMassage)
{
    std::println("\033[36m{}\033[31m{}\033[0m", prefix, errorMassage);
}


}

#ifdef _WIN32
#define TODO(x) \
    do { \
        mlog::log("TODO: ", mlog::Blue, f("\n   {}:{} \033[{}m{}\033[0m", "filePath"/*__FILE_NAME__*/, __LINE__, mlog::Red, x).c_str()); \
        exit(1); \
    }while(0)
#else 
#define TODO(x) \
    do { \
        mlog::log("TODO: ", mlog::Blue, f("\n   {}:{} \033[{}m{}\033[0m", __FILE_NAME__, __LINE__, mlog::Red, x).c_str()); \
        exit(1); \
    }while(0)
#endif

#endif //MASTERLOGGER
