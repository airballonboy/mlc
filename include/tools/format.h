#pragma once
#include "tokenizer.hpp"
#include <iomanip>
#include <sstream>
#include <stdexcept>


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

inline std::string TokenType_to_string(ml::TokenType tt) {
	std::string name;
	switch (tt) {
		case ml::TokenType::UNKNOWN:		 	name = "UNKNOWN";break;
		case ml::TokenType::INT32_T:		 	name = "INT32_T";break;
		case ml::TokenType::INT64_T:		 	name = "INT64_T";break;
		case ml::TokenType::INT16_T:			name = "INT16_T";break;
		case ml::TokenType::STRING_T: 			name = "STRING_T";break;
		case ml::TokenType::CHAR_T: 			name = "CHAR_T";break;
		case ml::TokenType::FLOAT_T: 			name = "FLOAT_T";break;
		case ml::TokenType::SIZE_T: 			name = "SIZE_T";break;
		case ml::TokenType::IDENT: 				name = "IDENT";break;
		case ml::TokenType::FUNC: 				name = "FUNC";break;
		case ml::TokenType::EXTRN: 				name = "EXTRN";break;
		case ml::TokenType::INCLUDE: 			name = "INCLUDE";break;
		case ml::TokenType::IMPORT: 			name = "IMPORT";break;
		case ml::TokenType::FROM: 				name = "FROM";break;
		case ml::TokenType::IF: 				name = "IF";break;
		case ml::TokenType::ELIF: 				name = "ELIF";break;
		case ml::TokenType::ELSE: 				name = "ELSE";break;
		case ml::TokenType::RETURN: 			name = "RETURN";break;
		case ml::TokenType::INT_LIT: 			name = "INT_LIT";break;
		case ml::TokenType::STRING_LIT: 		name = "STRING_LIT";break;
		case ml::TokenType::COMMENT: 			name = "COMMENT";break;
		case ml::TokenType::OPEN_PAREN: 		name = "OPEN_PAREN";break;
		case ml::TokenType::CLOSE_PAREN: 		name = "CLOSE_PAREN";break;
		case ml::TokenType::OPEN_CURLY: 		name = "OPEN_CURLY";break;
		case ml::TokenType::CLOSE_CURLY: 		name = "CLOSE_CURLY";break;
		case ml::TokenType::S_QOUTE: 			name = "S_QOUTE";break;
		case ml::TokenType::D_QOUTE: 			name = "D_QOUTE";break;
		case ml::TokenType::AND: 				name = "AND";break;
		case ml::TokenType::EQUAL: 				name = "EQUAL";break;
		case ml::TokenType::PLUS: 				name = "PLUS";break;
		case ml::TokenType::STAR: 				name = "STAR";break;
		case ml::TokenType::MINUS: 				name = "MINUS";break;
		case ml::TokenType::FSLASH: 			name = "FSLASH";break;
		case ml::TokenType::BSLASH: 			name = "BSLASH";break;
		case ml::TokenType::SEMI: 				name = "SEMI";break;
		case ml::TokenType::COLON: 				name = "COLON";break;
		case ml::TokenType::PERCENT: 			name = "PERCENT";break;
		case ml::TokenType::DOT: 				name = "DOT";break;
		case ml::TokenType::BITWISE: 			name = "BITWISE";break;
		case ml::TokenType::COMMA: 				name = "COMMA";break;
		case ml::TokenType::L_BRACKET: 			name = "L_BRACKET";break;
		case ml::TokenType::R_BRACKET:			name = "R_BRACKET";break;
		case ml::TokenType::HASH: 				name = "HASH";break;
		case ml::TokenType::L_THAN: 			name = "L_THAN";break;
		case ml::TokenType::G_THAN: 			name = "G_THAN";break;
		case ml::TokenType::AT_SIGN: 			name = "AT_SIGN";break;
		case ml::TokenType::EXCLAMATION_MARK:	name = "EXCLAMATION_MARK";break;
		case ml::TokenType::END_OF_FILE:		name = "END_OF_FILE";break;
	}
	return name;
}

inline std::string optional_to_string(std::optional<std::string> opt){
	if (opt.has_value())
		return opt.value();
	return std::string("null");
}

inline std::string Token_to_string(ml::Token t){
	// all weird stuff for cool colors :)
	return f("\033[35mToken\033[0m(\033[36m{}\033[0m, \033[32m{}\033[0m) at({}:{})", TokenType_to_string(t.type), optional_to_string(t.value),
		  		t.loc.line, t.loc.col);
}

