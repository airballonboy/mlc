#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <print>
#include <string>
#include <vector>
#include "codegen/gnu_asm_x86_64.h"
#include "parser.h"
#include "types.h"
#include "tools/file.h"
#include "codegen/ir.h"
#include "tools/logger.h"
#include "tools/format.h"
#include "lexar.h"

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
    if (strstr(inputFile.c_str(), ".mlang")) input_no_extention = inputFile.substr(0, inputFile.size() - 6);

    std::vector<char *> args;
    while (argc > 0) {
        args.push_back(shift_args(&argc, &argv));
    }
    for (auto& arg : args) {
        std::println("{}", arg);
    }

	auto lexar = Lexar(readFileToString(inputFile), inputFile);

    Parser parser(&lexar);

    auto prog = parser.parse();

    //ir   compiler(prog);
    gnu_asm compiler(prog);
    compiler.compileProgram();

    system(f("as {}.as -o {}.o"  , input_no_extention, input_no_extention).c_str());
    printf(f("as {}.as -o {}.o\n", input_no_extention, input_no_extention).c_str());
    system(f("gcc {}.o -o {}"    , input_no_extention, input_no_extention).c_str());
    printf(f("gcc {}.o -o {}\n"  , input_no_extention, input_no_extention).c_str());

	return 0;
}
