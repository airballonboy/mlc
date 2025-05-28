#ifndef DATATYPES
#define DATATYPES

#include <ios>
#include <vector>

namespace ML {

typedef struct {
	char* buffer;
	std::streamsize size;
}File;

typedef struct {
	long id;
	std::string data;
	long line;
	long location;
}Token;
typedef std::vector<Token> Tokens;

enum Type{
	INT,
	BOOL,
	FLOAT,
	CHAR,
	STRING,
};

typedef struct{
	std::string name;
}Label;
typedef struct{
	Type   type;
	Label  label;
	void*  value;
	size_t size;
}Variable;



}

#endif //DATATYPES
