#include "tokenizer.hpp"
#include "dataTypes.hpp"
#include <cstring>

static long currentId = 0;
namespace ML {

Tokens fileToTokens(File f) {
	long currentLine = 1, currentLoc = 1, firstCharLoc = 1;
	int i = 0;
	Tokens tokenVec;
	for(char* ptr = f.buffer; (*ptr != '\0'); ++ptr){
		Token t;
		char currentBuffer[100];
		if((*ptr != ' ')&&(*ptr != '\n')){
			currentBuffer[i++] = *ptr;
			if ((i-1) == 0)
				firstCharLoc = currentLoc;
			currentLoc++;
		}else if (currentBuffer[0] != 0){
			t = Token{
				.id   = ++currentId,
				.data = (std::string)currentBuffer, 
				.line = currentLine,
				.location = firstCharLoc
			};
			tokenVec.push_back(t);
			i = 0;
			std::memset(currentBuffer, 0, sizeof(currentBuffer));
		}
		if (*ptr == ' '){
			currentLoc++;
		}else if (*ptr == '\n'){
			currentLine++;
			currentLoc = 1;
		}

	}

	return tokenVec;
}

}
