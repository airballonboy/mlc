#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <print>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include "codegen/gnu_asm_x86_64.h"
#include "context.h"
#include "parser.h"
#include "types.h"
#include "tools/file.h"
#include "codegen/ir.h"
#include "tools/logger.h"
#include "tools/format.h"
#include "lexar.h"

using std::println;
using std::print;


enum class Platform {
    ir,
    gnu_asm_86_64,
};


#ifndef GCC
#ifdef _WIN32
#define GCC "gcc.exe -static-libgcc"
#else 
#define GCC "gcc"
#endif
#endif
#ifndef EXECUTABLE_EXTENSION
#ifdef _WIN32
#define EXECUTABLE_EXTENSION ".exe" 
#else 
#define EXECUTABLE_EXTENSION "" 
#endif
#endif
char* shift_args(int *argc, char ***argv) {
	assert("no more args" && *argc > 0);
	char *result = (**argv);
	(*argv) += 1;
	(*argc) -= 1;
	return result;
}

template<typename... _Args>
int cmd(std::format_string<_Args...> __fmt, _Args&&... __args) {
    print("Running: ");
    println(__fmt, std::forward<_Args>(__args)...);
    int result = std::system(std::format(__fmt, std::forward<_Args>(__args)...).c_str());
    #ifdef _WIN32
    return result;  // On Windows, std::system returns exit code directly
    #else
    return WEXITSTATUS(result);  // POSIX-style exit status extraction
    #endif

}

// should save token id in the token

int main(int argc, char* argv[])
{
    bool run = false;
    Platform platform = Platform::gnu_asm_86_64;
	std::string programName = shift_args(&argc, &argv);
	if (argc == 0) {
		logger::error("PROGRAM: ", "incorrect usage");
		logger::error("PROGRAM: ", "correct usage is");
		logger::log("   ", logger::Blue, f("{} input.mlang [-run [-a arg0 arg1 arg2 ...]]", programName).c_str());
		return 1;
	}
    std::string inputFile = shift_args(&argc, &argv);

    std::filesystem::path inputFilePath(inputFile);
    input_no_extention = inputFilePath.stem().string();
    input_path = inputFilePath.parent_path().string();
    build_path = input_path+"/.build";
    std::string output_path = build_path+"/output";

    // Should add the libmlang path to here
    ctx.includePaths.push_back(".");
	#ifdef WIN32
    ctx.includePaths.push_back("D:/ahmed/dev/cpp/mlc/test");
	#else 
    ctx.includePaths.push_back("/home/ahmed/dev/cpp/mlc/test");
	#endif
    if (!std::filesystem::exists(build_path)) {
        if (std::filesystem::create_directory(build_path)) {
            println("Directory created: {}", build_path);
        } else {
            println(stderr, "Failed to create directory: {}", build_path);
        }
    } else {
        println("Directory already exists: {}", build_path);
    }

    std::vector<std::string_view> args;
    while (argc > 0) {
        args.push_back(shift_args(&argc, &argv));
    }

    for (int i = 0; i < args.size(); i++) {
        if (args[i] == "-o") {
            if (i+1 < args.size())
                output_path = args[++i];
            else {
                println(stderr, "-o requires output path after it");
                exit(1);
            }
        }else if (args[i] == "-I") {
            if (i+1 < args.size()) {
                ctx.includePaths.push_back(args[++i]);
            }else {
                println(stderr, "-I requires search path for includes, imports, libs and binarys after it");
                exit(1);
            }
        }else if (args[i] == "-t") {
            if (i+1 < args.size()) {
                std::string_view plat_name = args[++i];
                if(plat_name == "gnu_x86_64") { platform = Platform::gnu_asm_86_64; continue; }
                if(plat_name == "ir")         { platform = Platform::ir;            continue; }
            }else {
                println(stderr, "-t requires platform name after it");
                exit(1);
            }
        }else if (args[i] == "-run") {
            run = true;
        }
    }

	auto lexar = Lexar(readFileToString(inputFile), inputFile);

    Parser parser(&lexar);

    auto prog = parser.parse();

    switch (platform) {
        case Platform::ir: {
            ir compiler_ir(prog);
            compiler_ir.compileProgram();
        }break;
        default:
        case Platform::gnu_asm_86_64: {
            gnu_asm compiler(prog);
            compiler.compileProgram();
            cmd("{} -static -x assembler {}/{}.s -o {} -lc", GCC, build_path, input_no_extention, output_path);
        }break;
    }



    
    if (run)
        return cmd("{}{}", output_path, EXECUTABLE_EXTENSION);
}
