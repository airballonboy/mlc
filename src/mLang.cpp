#include "tokenizer.hpp"
#include <array>
#include <cassert>
#include <cstdio>
#include <string>
#include <iostream>


char* shift_args(int *argc, char ***argv) {
	assert("no more args" && *argc > 0);
    char *result = (**argv);
    (*argv) += 1;
    (*argc) -= 1;
    return result;
}

int main(int argc, char* argv[])
{
	std::string programName = shift_args(&argc, &argv);
    if (argc == 0) {
        std::cerr << "Incorrect usage. Correct usage is..." << std::endl;
        std::cerr << "hydro <input.hy>" << std::endl;
        return EXIT_FAILURE;
    }
	std::string inputFile = shift_args(&argc, &argv);

	auto token = ml::tokenizeFile(inputFile);

	int it = 0;
	std::array<ml::Token, 400>tarr;
	for (auto& t : token) {
		tarr[it++] = t;
	}

	for (int i = 0;i < 100;i++);

    return EXIT_SUCCESS;
}
