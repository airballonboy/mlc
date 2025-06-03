#ifndef TOKENIZER_HEADER
#define TOKENIZER_HEADER
#include <string>
#include <optional>
#include <vector>

namespace ml {

enum class TokenType
{
	UNKNOWN = 0,
	//Types
	INT32_T, INT64_T, INT16_T,
	STRING_T, CHAR_T, FLOAT_T,
	SIZE_T,
	//Keywords
    IDENT, FUNC, EXTRN,
	INCLUDE, IMPORT, FROM,
	//literals
    INT_LIT, STRING_LIT,
	//Signs
	OPEN_PAREN, CLOSE_PAREN,
    OPEN_CURLY, CLOSE_CURLY,
	S_QOUTE, D_QOUTE, AND,
    EQUAL, PLUS,
    STAR, MINUS,
    FSLASH, BSLASH,
    IF, ELIF, ELSE,
    RETURN,
    SEMI, COLON, PERCENT,
	DOT, BITWISE, COMMA,
	L_BRACKET, R_BRACKET,
	HASH, L_THAN, G_THAN,
	AT_SIGN, EXCLAMATION_MARK,	
};


struct Token
{
    TokenType type;
    std::optional<std::string> value{};
};

std::vector<Token> tokenizeFile(std::string filePath);
std::vector<Token> tokenizeString(std::string code);
Token stringToToken(std::string s);
Token breakerToToken(char c);

bool isbreaker(char c);

}


#endif //TOKENIZER_HEADER
