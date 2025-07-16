#ifndef MASTERLOGGER
#define MASTERLOGGER

#include <cstdio>

namespace logger {


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
    printf("\033[%dm%s\033[0m\n", color, text);
}
inline void log(const char* prefix, const color color, const char* text)
{
    printf("\033[36m%s\033[%dm%s\033[0m\n", prefix, color, text);
}

inline void success(const char* text)
{
    printf("\033[32m%s\033[0m\n", text);
}
inline void success(const char* prefix, const char* successMassage)
{
    printf("\033[36m%s\033[32m%s\033[0m\n", prefix, successMassage);
}


inline void error(const char* error)
{
    printf("\033[31m%s\033[0m\n", error);
}
inline void error(const char* prefix, const char* errorMassage)
{
    printf("\033[36m%s\033[31m%s\033[0m\n", prefix, errorMassage);
}


}

#include "tools/format.h"
#define TODO(x) \
    do { \
        logger::log("TODO: ", logger::Blue, f("\n   {}:{} \033[{}m{}\033[0m", __FILE_NAME__, __LINE__, logger::Red, x).c_str()); \
        exit(1); \
    }while(0)
#endif //MASTERLOGGER
