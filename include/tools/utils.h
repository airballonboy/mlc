#pragma once


#include <string>
#include <format>
#include <print>
#include <unordered_map>
#include <cassert>
#include <vector>

enum class BuildTarget {
    ir,
    gnu_asm_86_64,
};
inline std::unordered_map<std::string, BuildTarget> PLATFORMS = {
    {"gnu_x64_64", BuildTarget::gnu_asm_86_64},
    {"ir",         BuildTarget::ir},
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
template<typename... _Args>
inline std::tuple<int, std::string> cmd_with_output(std::format_string<_Args...> __fmt, _Args&&... __args) {
    int status = 0;
    std::string output{};
    std::array<char, 4096> buffer = {};
    std::string command = std::format(__fmt, std::forward<_Args>(__args)...);
#ifdef _WIN32
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) {
        std::println(" _popen() failed");
        std::exit(1);
    }

    while (fgets(buffer.data(), (int)buffer.size(), pipe)) {
        output += buffer.data();
    }

    status = _pclose(pipe);
#else
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::println("popen() failed");
        exit(1);
    }

    while (fgets(buffer.data(), buffer.size(), pipe)) {
        output += buffer.data();
    }

    status = pclose(pipe);
    status = WEXITSTATUS(status);  // POSIX-style exit status extraction
#endif
    return {status, output};
}
inline std::string& escape_new_lines(std::string& str) {
    size_t i = str.find('\n');
    while (i != std::string::npos) {
        str.replace(i, 1, "\\n");
        i = str.find('\n', i+2);
    }
    return str;
}
