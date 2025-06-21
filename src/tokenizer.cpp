#include "tokenizer.hpp"
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>


size_t current_token_count = 0;

std::vector<ml::Token> ml::tokenizeFile(std::string filePath) {
	std::vector<ml::Token> tokens;
	std::string fileContent;
    std::stringstream contents_stream;
	std::fstream input(filePath, std::ios::in);
	contents_stream << input.rdbuf();
	fileContent = contents_stream.str();
	input.close();
	contents_stream.clear();
	tokens = tokenizeString(fileContent, filePath);
	return tokens;
}

std::vector<ml::Token> ml::tokenizeString(std::string code, std::string filePath) {
	std::vector<ml::Token> tokens;
	ml::Loc currentLoc;
	currentLoc.file = filePath;
	size_t i = 0;
	while (i < code.size()){
		auto& c = code[i];
		// Checks for single line comments
		if (i+1 < code.size() && code[i] == '/' && code[i+1] == '/'){
			std::string s;
			Loc begin = currentLoc;
			while (code[i] != '\n' && i < code.size()){
				s.push_back(code[i]);
				i++;
				currentLoc.col++;
			}
			tokens.push_back({current_token_count++, begin, TokenType::COMMENT, s});
		// Checks for multi line comments
		}else if(i+3 < code.size() && code[i] == '/' && code[i+1] == '*'){
			std::string s;
			Loc begin = currentLoc;
			while (i+1 < code.size() && !(code[i] == '*' && code[i+1] == '/')){
				s.push_back(code[i]);
				i++;
				currentLoc.col++;
			}
			s.push_back(code[i]);
			i++;
			currentLoc.col++;
			s.push_back(code[i]);
			i++;
			currentLoc.col++;
			tokens.push_back({current_token_count++, begin, TokenType::COMMENT, s});
		// Checks if the current char is a " and the previous one wasn't a STRING_LIT
		}else if (!tokens.empty() && tokens[tokens.size()-1].type == TokenType::D_QOUTE && tokens[tokens.size()-2].type != TokenType::STRING_LIT){
			std::string s;
			Loc begin = currentLoc;
			while (code[i] != '\"' && i < code.size()) {
				s.push_back(code[i]);
				i++;
				currentLoc.col++;
			}
			tokens.push_back({current_token_count++, begin, TokenType::STRING_LIT, s});
		// Checks if the current char is a < and continues until it finds >
		}else if(!tokens.empty() && tokens[tokens.size() - 1].type == TokenType::L_THAN){
			std::string s;
			while(code[i] != '>' && i < code.size()) {
				s.push_back(code[i]);
				i++;
				currentLoc.col++;
			}
			tokens.push_back({current_token_count++, currentLoc, TokenType::STRING_LIT, s});
		}else if (isdigit(c)){
			std::string s;
			s.push_back(c);
			i++;
			currentLoc.col++;
			while (isdigit(code[i])) {
				s.push_back(code[i++]);
				currentLoc.col++;
			}
			tokens.push_back({current_token_count++, currentLoc, TokenType::INT_LIT, s});
		}else if (isbreaker(c)){
			tokens.push_back(breakerToToken(c, currentLoc));		
			i++;
			currentLoc.col++;
		}else if (isalpha(c)) {
			Loc begin = currentLoc;
			std::string s;
			s.push_back(c);
			i++;
			currentLoc.col++;
			while ((i < code.size()) && (isalpha(code[i]) || code[i] == '_')) {
				s.push_back(code[i++]);
				currentLoc.col++;
			}
			tokens.push_back(stringToToken(s, begin));
		}else if (isspace(c)){
			if(c == '\n') {
				currentLoc.line++;
				currentLoc.col = 1;
				i++;
			}else {
				currentLoc.col++;
				i++;
			}
		}else {
			std::cout << "unknown: " << code[i] << "\n";
		}

	}
	tokens.push_back({current_token_count, currentLoc, TokenType::END_OF_FILE});
	

	return tokens;
}
ml::Token ml::stringToToken(std::string s, Loc currentLoc) {
	if(s == "return") 	return Token{current_token_count++, currentLoc, TokenType::RETURN	, s};
	if(s == "extern")	return Token{current_token_count++, currentLoc, TokenType::EXTRN	, s};
	if(s == "import") 	return Token{current_token_count++, currentLoc, TokenType::IMPORT	, s};
	if(s == "from") 	return Token{current_token_count++, currentLoc, TokenType::FROM		, s};
	if(s == "include") 	return Token{current_token_count++, currentLoc, TokenType::INCLUDE	, s};
	if(s == "func")		return Token{current_token_count++, currentLoc, TokenType::FUNC		, s};
	if(s == "int") 		return Token{current_token_count++, currentLoc, TokenType::INT32_T	, s};
	if(s == "i64") 		return Token{current_token_count++, currentLoc, TokenType::INT64_T	, s};
	if(s == "i32") 		return Token{current_token_count++, currentLoc, TokenType::INT32_T	, s};
	if(s == "i16") 		return Token{current_token_count++, currentLoc, TokenType::INT16_T	, s};
	if(s == "string") 	return Token{current_token_count++, currentLoc, TokenType::STRING_T	, s};
	if(s == "char") 	return Token{current_token_count++, currentLoc, TokenType::CHAR_T	, s};
	if(s == "float") 	return Token{current_token_count++, currentLoc, TokenType::FLOAT_T	, s};
	if(s == "size") 	return Token{current_token_count++, currentLoc, TokenType::SIZE_T	, s};

	return Token{current_token_count++, currentLoc, TokenType::IDENT, s};
}
ml::Token ml::breakerToToken(char c, Loc currentLoc) {
	if(c == '\'') return Token{current_token_count++, currentLoc, TokenType::S_QOUTE			, std::string(1, c)};
	if(c == '\"') return Token{current_token_count++, currentLoc, TokenType::D_QOUTE			, std::string(1, c)};
	if(c == '\\') return Token{current_token_count++, currentLoc, TokenType::BSLASH				, std::string(1, c)};
	if(c == '/')  return Token{current_token_count++, currentLoc, TokenType::FSLASH				, std::string(1, c)};
	if(c == '{')  return Token{current_token_count++, currentLoc, TokenType::OPEN_CURLY			, std::string(1, c)};
	if(c == '}')  return Token{current_token_count++, currentLoc, TokenType::CLOSE_CURLY		, std::string(1, c)};
	if(c == '(')  return Token{current_token_count++, currentLoc, TokenType::OPEN_PAREN			, std::string(1, c)};
	if(c == ')')  return Token{current_token_count++, currentLoc, TokenType::CLOSE_PAREN		, std::string(1, c)};
	if(c == '&')  return Token{current_token_count++, currentLoc, TokenType::AND				, std::string(1, c)};
	if(c == '*')  return Token{current_token_count++, currentLoc, TokenType::STAR				, std::string(1, c)};
	if(c == '-')  return Token{current_token_count++, currentLoc, TokenType::MINUS				, std::string(1, c)};
	if(c == '+')  return Token{current_token_count++, currentLoc, TokenType::PLUS				, std::string(1, c)};
	if(c == '=')  return Token{current_token_count++, currentLoc, TokenType::EQUAL				, std::string(1, c)};
	if(c == ';')  return Token{current_token_count++, currentLoc, TokenType::SEMI				, std::string(1, c)};
	if(c == '/')  return Token{current_token_count++, currentLoc, TokenType::FSLASH				, std::string(1, c)};
	if(c == '^')  return Token{current_token_count++, currentLoc, TokenType::BITWISE			, std::string(1, c)};
	if(c == '%')  return Token{current_token_count++, currentLoc, TokenType::PERCENT			, std::string(1, c)};
	if(c == ':')  return Token{current_token_count++, currentLoc, TokenType::COLON				, std::string(1, c)};
	if(c == '.')  return Token{current_token_count++, currentLoc, TokenType::DOT				, std::string(1, c)};
	if(c == ',')  return Token{current_token_count++, currentLoc, TokenType::COMMA				, std::string(1, c)};
	if(c == '[')  return Token{current_token_count++, currentLoc, TokenType::L_BRACKET			, std::string(1, c)};
	if(c == ']')  return Token{current_token_count++, currentLoc, TokenType::R_BRACKET			, std::string(1, c)};
	if(c == '#')  return Token{current_token_count++, currentLoc, TokenType::HASH				, std::string(1, c)};
	if(c == '<')  return Token{current_token_count++, currentLoc, TokenType::L_THAN				, std::string(1, c)};
	if(c == '>')  return Token{current_token_count++, currentLoc, TokenType::G_THAN				, std::string(1, c)};
	if(c == '@')  return Token{current_token_count++, currentLoc, TokenType::AT_SIGN			, std::string(1, c)};
	if(c == '!')  return Token{current_token_count++, currentLoc, TokenType::EXCLAMATION_MARK	, std::string(1, c)};

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
