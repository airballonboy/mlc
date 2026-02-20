#pragma once
#include <cctype>
#include <iomanip>
#include <ios>
#include <sstream>
#include <optional>
#include <stdexcept>
#include <type_traits>

namespace mlog {

struct FormatSpec {
    char fill = ' ';
    int width = 0;
    char type = 'd';
};

static FormatSpec parse_format_spec(const std::string& fmt, size_t& i) {
    FormatSpec spec;
    if (fmt[i] == ':') {
        ++i;
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
        if (i < fmt.size() && std::isalpha(fmt[i])) {
            spec.type = fmt[i++];
        }
    }
    if (i >= fmt.size() || fmt[i] != '}') {
        fputs("Malformed format specifier", stderr);
        exit(1);
    }
    ++i;
    return spec;
}
template<typename T>
std::string format_unsigned_integral(T val, char type) {
    static_assert(std::is_unsigned_v<T>, "format_unsigned_integral requires unsigned type");

    if (type == 'b') {
        if (val == 0) return "0";
        std::string tmp{};
        while (val) {
            tmp.insert(tmp.begin(), (val & 1) ? '1' : '0');            
            val >>= 1;
        }
        return tmp;
    } else {
        std::ostringstream tmp{};
        switch (type) {
        case 'X': 
            tmp << std::hex << std::uppercase << val;
            break;
        case 'x': 
            tmp << std::hex << std::nouppercase << val;
            break;
        default:
            tmp << std::dec << val;
        }
        return tmp.str();
    }
}
template<typename T>
std::string format_signed_integral(T val, char type) {
    static_assert(std::is_signed_v<T>  , "format_signed_integral requires signed type");

    if (type == 'd') {
        std::ostringstream tmp{};
        tmp << std::dec << val;
        return tmp.str();
    } else {
        using U = std::make_unsigned_t<T>;        
        U unsigned_val = static_cast<U>(val);
        return format_unsigned_integral(unsigned_val, type);
    }
}
template<typename T>
inline std::string format_integral(T val, char type) {
    static_assert(std::is_integral_v<T>, "format_integral requires integral type");
    if constexpr (std::is_unsigned_v<T>) {
        return format_unsigned_integral(val, type);
    } else {
        return format_signed_integral(val, type);
    }
}

template<typename T>
void append_arg(std::ostringstream& oss, const T& arg, const FormatSpec& spec = {}) {
    if constexpr (std::is_integral_v<T>) {
        std::string out{};
        bool negative = false;
        if constexpr (std::is_signed_v<T>) {
            if (spec.type) {
                if (arg < 0)
                    negative = true;
            }
        }
        if (negative) {
            using U = std::make_unsigned_t<T>;
            U unsigned_arg = static_cast<U>(-(arg + 0));
            out = format_unsigned_integral(unsigned_arg, 'd');
        } else {
            out = format_integral(arg, spec.type);
        }
        size_t len = out.size() + (negative ? 1 : 0);
        size_t pad = (spec.width > len) ? (spec.width - len) : 0;

        if (spec.fill == '0') {
            if (negative) oss << '-';
            for (size_t i = 0; i < pad; i++) oss << '0';
        } else {
            for (size_t i = 0; i < pad; i++) oss << spec.fill;
            if (negative) oss << '-';
        }
        oss << out;
    } else {
        if (spec.width > 0) {
            oss << std::setfill(spec.fill) << std::setw(spec.width);
        }
        oss << arg;
    }
}

inline void format_recursive(std::ostringstream& oss, const std::string& fmt, size_t& i) {
    while (i < fmt.size()) {
        if (fmt[i] == '{' && i + 1 < fmt.size() && fmt[i + 1] == '}') {
            fputs("Too few arguments for format string", stderr);
            exit(1);
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
    fputs("Too many arguments for format string", stderr);
    exit(1);
}


template<typename... Args>
std::string format(const std::string& fmt, const Args&... args) {
    std::ostringstream oss;
    size_t i = 0;
    format_recursive(oss, fmt, i, args...);
    return oss.str();
}

}


inline std::string optional_to_string(std::optional<std::string> opt){
    if (opt.has_value())
        return opt.value();
    return std::string("(nil)");
}


//important for std::string
#define appendf(...) append(mlog::format(__VA_ARGS__))

