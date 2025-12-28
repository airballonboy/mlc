#pragma once


#include <string>
#include <format>
#include <print>
#include <unordered_map>
#include <cassert>
#include <vector>

enum class Platform {
    ir,
    gnu_asm_86_64,
};
std::unordered_map<std::string, Platform> PLATFORMS = {
    {"gnu_x64_64", Platform::gnu_asm_86_64},
    {"ir",         Platform::ir},
};


#ifndef CC
#ifdef _WIN32
#define CC "gcc -lmsvcrt"
#else 
#define CC "gcc -lc"
#endif
#endif
#ifndef EXECUTABLE_EXTENSION
#ifdef _WIN32
#define EXECUTABLE_EXTENSION ".exe" 
#else 
#define EXECUTABLE_EXTENSION "" 
#endif
#endif

inline char* shift_args(int *argc, char ***argv) {
    assert("no more args" && *argc > 0);
    char *result = (**argv);
    (*argv) += 1;
    (*argc) -= 1;
    return result;
}
inline std::string_view shift_args(std::vector<std::string_view>& args) {
    assert("no more args" && args.size() > 0);
    std::string_view result = args[0];
    
    args.erase(args.begin());

    return result;
}

template<typename... _Args>
inline int cmd(std::format_string<_Args...> __fmt, _Args&&... __args) {
    std::print("Running: ");
    std::println(__fmt, std::forward<_Args>(__args)...);
    int result = std::system(std::format(__fmt, std::forward<_Args>(__args)...).c_str());
    #ifdef _WIN32
    return result;  // On Windows, std::system returns exit code directly
    #else
    return WEXITSTATUS(result);  // POSIX-style exit status extraction
    #endif

}
