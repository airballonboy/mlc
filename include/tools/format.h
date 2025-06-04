#pragma once
#include <format>

// f is an alias to std::format()
template<typename... Args>
std::string f(std::format_string<Args...> fmt, Args&&... args) {
    return std::format(fmt, std::forward<Args>(args)...);
}


