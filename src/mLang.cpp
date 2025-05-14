#include "dataTypes.hpp"
#include "fileOperations.hpp"
#include "tokenizer.hpp"
#include <cassert>
#include <iostream>
#include <string>


char* shift_args(int *argc, char ***argv) {
	assert("no more args" && *argc > 0);
    char *result = (**argv);
    (*argv) += 1;
    (*argc) -= 1;
    return result;
}

int main(int argc, char** argv) 
{
	std::string programName = shift_args(&argc, &argv);
	ML::File current = ML::openFile(shift_args(&argc, &argv));
	ML::Tokens ts = ML::fileToTokens(current);
	for (auto& t : ts){
		//std::cout << "--------------------------\n";
		//std::cout << "id: "<< t.id << "\n";
		//std::cout << "loc: "<< t.location << "\n";
		std::cout << "data: "<< t.data << "\n";
		//std::cout << "line: "<< t.line << "\n";
	}
}
