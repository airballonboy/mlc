#include "tokenizer.hpp"
#include <array>
#include <cassert>
#include <cstdio>
#include <string>
#include "tools/logger.h"
#include "tools/format.h"

char* shift_args(int *argc, char ***argv) {
	assert("no more args" && *argc > 0);
	char *result = (**argv);
	(*argv) += 1;
	(*argc) -= 1;
	return result;
}
// should save token id in the token

int main(int argc, char* argv[])
{
	std::string programName = shift_args(&argc, &argv);
	if (argc == 0) {
		logger::error("PROGRAM: ", "incorrect usage");
		logger::error("PROGRAM: ", "correct usage is");
		logger::log("   ", logger::Blue, f("{} input.mlang [-run [-a arg0 arg1 arg2 ...]]", programName).c_str());
		return 1;
	}
	std::string inputFile = shift_args(&argc, &argv);

	auto token = ml::tokenizeFile(inputFile);

	int it = 0;
	for (auto& t : token) {
		std::cout << f("{}: {}\n", it++, Token_to_string(t));
	}

	return 0;
}
