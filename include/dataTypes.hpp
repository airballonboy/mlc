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


}

#endif //DATATYPES
