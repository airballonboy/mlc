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
	IF, ELIF, ELSE,
	RETURN,
	//literals
	INT_LIT, STRING_LIT, COMMENT,
	//Signs
	OPEN_PAREN, CLOSE_PAREN,
	OPEN_CURLY, CLOSE_CURLY,
	S_QOUTE, D_QOUTE, AND,
	EQUAL, PLUS,
	STAR, MINUS,
	FSLASH, BSLASH,
	SEMI, COLON, PERCENT,
	DOT, BITWISE, COMMA,
	L_BRACKET, R_BRACKET,
	HASH, L_THAN, G_THAN,
	AT_SIGN, EXCLAMATION_MARK,
	END_OF_FILE,
};

struct Loc{
	size_t line = 1;
	size_t col  = 1;
	std::string file;
};
struct Token
{
	size_t id = 0;
	Loc loc = {};
	TokenType type = {};
	std::optional<std::string> value{};
};

std::vector<Token> tokenizeFile(std::string filePath);
std::vector<Token> tokenizeString(std::string code, std::string filePath);
Token stringToToken(std::string s, Loc currentLoc);
Token breakerToToken(char c, Loc currentLoc);

bool isbreaker(char c);

}


#endif //TOKENIZER_HEADER
