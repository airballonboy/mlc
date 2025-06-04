#include "tokenizer.hpp"
#include <array>
#include <cassert>
#include <cstdio>
#include <string>
#include <format>
#include "tools/logger.h"
#include "tools/format.h"

char* shift_args(int *argc, char ***argv) {
	assert("no more args" && *argc > 0);
	char *result = (**argv);
	(*argv) += 1;
	(*argc) -= 1;
	return result;
}

 int hewo;
int main(int argc, char* argv[])
{
	std::string programName = shift_args(&argc, &argv);
	if (argc == 0) {
		logger::error("PROGRAM: ", "incorrect usage");
		logger::error("PROGRAM: ", "correct usage is");
		logger::log("   ", logger::Blue, _f("{} input.mlang [-run [-a arg0 arg1 arg2 ...]]", programName));
		std::cout << _f("{}\n", hewo);
		return 1;
	}
	std::string inputFile = shift_args(&argc, &argv);

	auto token = ml::tokenizeFile(inputFile);

	int it = 0;
	std::array<ml::Token, 400>tarr;
	for (auto& t : token) {
		tarr[it++] = t;
	}

	for (int i = 0;i < 100;i++);

	return 0;
}
