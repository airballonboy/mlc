#include "tokenizer.hpp"
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>



std::vector<ml::Token> ml::tokenizeFile(std::string filePath) {
	std::vector<ml::Token> tokens;
	std::string fileContent;
    std::stringstream contents_stream;
	std::fstream input(filePath, std::ios::in);
	contents_stream << input.rdbuf();
	fileContent = contents_stream.str();
	input.close();
	contents_stream.clear();
	tokens = tokenizeString(fileContent);
	return tokens;
}

std::vector<ml::Token> ml::tokenizeString(std::string code) {
	std::vector<ml::Token> tokens;
	size_t i = 0;
	while (i < code.size()){
		auto& c = code[i];
		// Checks for single line comments
		if (i+1 < code.size() && code[i] == '/' && code[i+1] == '/'){
			std::string s;
			while (code[i] != '\n' && i < code.size()){
				s.push_back(code[i]);
				i++;
			}
			tokens.push_back({TokenType::COMMENT, s});
		// Checks for multi line comments
		}else if(i+3 < code.size() && code[i] == '/' && code[i+1] == '*'){
			std::string s;
			while (i+1 < code.size() && !(code[i] == '*' && code[i+1] == '/')){
				s.push_back(code[i]);
				i++;
			}
			s.push_back(code[i]);
			i++;
			s.push_back(code[i]);
			i++;
			tokens.push_back({TokenType::COMMENT, s});
		// Checks if the current char is a " and the previous one wasn't a STRING_LIT
		}else if (!tokens.empty() && tokens[tokens.size()-1].type == TokenType::D_QOUTE && tokens[tokens.size()-2].type != TokenType::STRING_LIT){
			std::string s;
			while (code[i] != '\"' && i < code.size()) {
				s.push_back(code[i]);
				i++;
			}
			tokens.push_back({TokenType::STRING_LIT, s});
		// Checks if the current char is a < and continues until it finds >
		}else if(!tokens.empty() && tokens[tokens.size() - 1].type == TokenType::L_THAN){
			std::string s;
			while(code[i] != '>' && i < code.size()) {
				s.push_back(code[i]);
				i++;
			}
			tokens.push_back({TokenType::STRING_LIT, s});
		}else if (isdigit(c)){
			std::string s;
			s.push_back(c);
			i++;
			while (isdigit(code[i])) {
				s.push_back(code[i++]);
			}
			tokens.push_back({TokenType::INT_LIT, s});
		}else if (isbreaker(c)){
			tokens.push_back(breakerToToken(c));		
			i++;
		}else if (isalpha(c)) {
			std::string s;
			s.push_back(c);
			i++;
			while ((i < code.size()) && (isalpha(code[i]) || code[i] == '_')) {
				s.push_back(code[i++]);
			}
			tokens.push_back(stringToToken(s));
		}else if (isspace(c)){
			i++;
		}else {
			std::cout << "unknown: " << code[i] << "\n";
		}

	}
	

	return tokens;
}
ml::Token ml::stringToToken(std::string s) {
	if(s == "return") 	return Token{TokenType::RETURN	, s};
	if(s == "extern")	return Token{TokenType::EXTRN	, s};
	if(s == "import") 	return Token{TokenType::IMPORT	, s};
	if(s == "from") 	return Token{TokenType::FROM	, s};
	if(s == "include") 	return Token{TokenType::INCLUDE	, s};
	if(s == "func")		return Token{TokenType::FUNC	, s};
	if(s == "int") 		return Token{TokenType::INT32_T	, s};
	if(s == "i64") 		return Token{TokenType::INT64_T	, s};
	if(s == "i32") 		return Token{TokenType::INT32_T	, s};
	if(s == "i16") 		return Token{TokenType::INT16_T	, s};
	if(s == "string") 	return Token{TokenType::STRING_T, s};
	if(s == "char") 	return Token{TokenType::CHAR_T	, s};
	if(s == "float") 	return Token{TokenType::FLOAT_T	, s};
	if(s == "size") 	return Token{TokenType::SIZE_T	, s};

	return Token{TokenType::IDENT, s};
}
ml::Token ml::breakerToToken(char c) {
	if(c == '\'') return Token{TokenType::S_QOUTE			, std::string(1, c)};
	if(c == '\"') return Token{TokenType::D_QOUTE			, std::string(1, c)};
	if(c == '\\') return Token{TokenType::BSLASH			, std::string(1, c)};
	if(c == '/')  return Token{TokenType::FSLASH			, std::string(1, c)};
	if(c == '{')  return Token{TokenType::OPEN_CURLY		, std::string(1, c)};
	if(c == '}')  return Token{TokenType::CLOSE_CURLY		, std::string(1, c)};
	if(c == '(')  return Token{TokenType::OPEN_PAREN		, std::string(1, c)};
	if(c == ')')  return Token{TokenType::CLOSE_PAREN		, std::string(1, c)};
	if(c == '&')  return Token{TokenType::AND				, std::string(1, c)};
	if(c == '*')  return Token{TokenType::STAR				, std::string(1, c)};
	if(c == '-')  return Token{TokenType::MINUS				, std::string(1, c)};
	if(c == '+')  return Token{TokenType::PLUS				, std::string(1, c)};
	if(c == '=')  return Token{TokenType::EQUAL				, std::string(1, c)};
	if(c == ';')  return Token{TokenType::SEMI				, std::string(1, c)};
	if(c == '/')  return Token{TokenType::FSLASH			, std::string(1, c)};
	if(c == '^')  return Token{TokenType::BITWISE			, std::string(1, c)};
	if(c == '%')  return Token{TokenType::PERCENT			, std::string(1, c)};
	if(c == ':')  return Token{TokenType::COLON				, std::string(1, c)};
	if(c == '.')  return Token{TokenType::DOT				, std::string(1, c)};
	if(c == ',')  return Token{TokenType::COMMA				, std::string(1, c)};
	if(c == '[')  return Token{TokenType::L_BRACKET			, std::string(1, c)};
	if(c == ']')  return Token{TokenType::R_BRACKET			, std::string(1, c)};
	if(c == '#')  return Token{TokenType::HASH				, std::string(1, c)};
	if(c == '<')  return Token{TokenType::L_THAN			, std::string(1, c)};
	if(c == '>')  return Token{TokenType::G_THAN			, std::string(1, c)};
	if(c == '@')  return Token{TokenType::AT_SIGN			, std::string(1, c)};
	if(c == '!')  return Token{TokenType::EXCLAMATION_MARK	, std::string(1, c)};

	return {};
}

std::vector<char> tokenBreakers = {
	'\'','\"','\\',
	'{','}','(',
	')','&','*',
	'-','+','=',
	';','/','^',
	'%',':','.',
	',','[',']',
	'#','<','>',
	'@','!'
};



bool ml::isbreaker(char c) {
	for (const char& t : tokenBreakers)
		if (t == c)
			return true;
	return false;
}
