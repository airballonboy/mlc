#include "tokenizer.hpp"
#include "dataTypes.hpp"
#include <cstdio>
#include <cstring>
#include <vector>

std::vector<char> tokenBreakers = {
	'\'','\"','\\',
	'{','}','(',
	')','&','*',
	'-','+','=',
	';','/','^',
	'%',':','.',
	',','[',']',
	'#','<','>'
};
std::vector<char> skipTokens = {
	' ', '\n','	',
};
static long currentId = 0;
namespace ML {

Tokens fileToTokens(File f) {

	//for (auto& t : tokenBreakers)
	//	printf("%c, ", t);
	//printf("\n");
	long currentLine = 1, currentLoc = 1, firstCharLoc = 1;
	int i = 0;
	Tokens tokenVec;
	char currentBuffer[100] = "";
	const auto& saveToken = [&](std::string buff, long line, long loc) {
		if (currentBuffer[0] != 0){
			Token t = Token{
				.id   	  = ++currentId,
				.data 	  = buff, 
				.line 	  = line,
				.location = loc
			};
			tokenVec.push_back(t);
			i = 0;
			std::memset(currentBuffer, 0, sizeof(currentBuffer));	
		}
	};
	const auto& checkBreakToken = [&](char c) -> bool{
		for (auto& t : tokenBreakers)
			if (c == t)
				return true;
		return false;
	};
	for(char* ptr = f.buffer; (*ptr != '\0'); ptr++){
		
		Token t;
		//-- Check if the token is space, tab or \n
		for (auto& t : skipTokens) {
			if (*ptr == t) {
				if (*ptr == ' '){
					currentLoc++;
				}else if (*ptr == '\n'){
					currentLine++;
					currentLoc = 1;
				}else if (*ptr == '	'){
					currentLoc += 4;
				}
				saveToken((std::string)currentBuffer, currentLine, currentLoc);
				goto skip;
			}		
		}
		//-- Check if the token is space, tab or \n
	

		if (checkBreakToken(*ptr) /*&& !checkBreakToken(*(ptr - 1))*/)
			saveToken((std::string)currentBuffer, currentLine, currentLoc);
		if (!checkBreakToken(*ptr) && checkBreakToken(*(ptr - 1)))
			saveToken((std::string)currentBuffer, currentLine, currentLoc);

		/*	only stacks the same breakToken
		for (auto& t : tokenBreakers) {
			if (*ptr == t && *(ptr - 1) != t)
				saveToken((std::string)currentBuffer, currentLine, currentLoc);
			if (*ptr != t && *(ptr - 1) == t)
				saveToken((std::string)currentBuffer, currentLine, currentLoc);
		}
		*/
	

		currentBuffer[i++] = *ptr;
		if ((i-1) == 0)
			firstCharLoc = currentLoc;
		currentLoc++;

		
		skip:
		(void)0;
	}

	return tokenVec;
}

}
