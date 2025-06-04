#pragma once
#include <format>

// _f is an alias to std::format().c_str()
template<typename... Args>
const char* _f(std::format_string<Args...> fmt, Args&&... args) {
    return std::format(fmt, std::forward<Args>(args)...).c_str();
}


