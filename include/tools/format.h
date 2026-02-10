#pragma once
#include <iomanip>
#include <sstream>
#include <optional>
#include <stdexcept>

//important for std::string
#define appendf(...) append(std::format(__VA_ARGS__))

// don't worry too much it's chat gpt implementation of std::format
struct FormatSpec {
    char fill = ' ';
    int width = 0;
};

static FormatSpec parse_format_spec(const std::string& fmt, size_t& i) {
    FormatSpec spec;
    if (fmt[i] == ':') {
        ++i; // skip ':'
        if (fmt[i] == '0') {
            spec.fill = '0';
            ++i;
        }
        std::string width_str;
        while (i < fmt.size() && isdigit(fmt[i])) {
            width_str += fmt[i++];
        }
        if (!width_str.empty()) {
            spec.width = std::stoi(width_str);
        }
    }
    if (i >= fmt.size() || fmt[i] != '}') {
        throw std::runtime_error("Malformed format specifier");
    }
    ++i; // skip '}'
    return spec;
}

template<typename T>
void append_arg(std::ostringstream& oss, const T& arg, const FormatSpec& spec = {}) {
    if (spec.width > 0) {
        oss << std::setfill(spec.fill) << std::setw(spec.width);
    }
    oss << arg;
}

inline void format_recursive(std::ostringstream& oss, const std::string& fmt, size_t& i) {
    while (i < fmt.size()) {
        if (fmt[i] == '{' && i + 1 < fmt.size() && fmt[i + 1] == '}') {
            throw std::runtime_error("Too few arguments for format string");
        } else {
            oss << fmt[i++];
        }
    }
}

template<typename T, typename... Args>
void format_recursive(std::ostringstream& oss, const std::string& fmt, size_t& i, const T& first, const Args&... rest) {
    while (i < fmt.size()) {
        if (fmt[i] == '{') {
            ++i; // skip '{'
            FormatSpec spec = parse_format_spec(fmt, i);
            append_arg(oss, first, spec);
            format_recursive(oss, fmt, i, rest...);
            return;
        } else {
            oss << fmt[i++];
        }
    }
    throw std::runtime_error("Too many arguments for format string");
}

template<typename... Args>
std::string f(const std::string& fmt, const Args&... args) {
    std::ostringstream oss;
    size_t i = 0;
    format_recursive(oss, fmt, i, args...);
    return oss.str();
}


inline std::string optional_to_string(std::optional<std::string> opt){
    if (opt.has_value())
        return opt.value();
    return std::string("null");
}

//inline std::string Token_to_string(Token t){
//  // all weird stuff for cool colors :)
//  return f("\033[35mToken\033[0m(\033[36m{}\033[0m, \033[32m{}\033[0m) at({}:{})", TokenType_to_string(t.type), optional_to_string(t.value),
//              t.loc.line, t.loc.col);
//}

