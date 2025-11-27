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
#include "flag.h"
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


#ifndef CC
#ifdef _WIN32
#define CC "gcc -lmsvcrt"
#else 
#define CC "gcc -lc"
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
std::string_view shift_args(std::vector<std::string_view>& args) {
    assert("no more args" && args.size() > 0);
    std::string_view result = args[0];
    
    args.erase(args.begin());

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
std::string programName;
std::string output_path;

std::unordered_map<std::string, Platform> PLATFORMS = {
    {"gnu_x64_64", Platform::gnu_asm_86_64},
    {"ir",         Platform::ir},
};
void print_platforms() {
    std::println("platforms");
    for (auto plat : PLATFORMS) {
        std::println("  {:8}", plat.first);
    }
}
void print_help_message() {
    mlog::log("Usage: ", mlog::Blue, f("{} input.mlang [options]", programName).c_str());
    println("\noptions:");
    for (auto& flag: FLAG_BASE::flags) {
        if (flag->name == "--help") continue;
        println("  {}", flag->desc);
    }
}

Flag<bool>                     help_flag2    ("--help", "");
Flag<bool>                     help_flag1    ("-h"    , "[-h] [--help]    prints this message");
Flag<bool>                     run_flag      ("-run"  , "[-run]           runs the program after compilation");
Flag<std::string>              output_flag   ("-o"    , "[-o executable]  outputs the program into the name provided");
Flag<std::string>              platform_flag ("-t"    , "[-t platform]    chooses which platform to compile to");
Flag<std::vector<std::string>> include_flag  ("-I"    , "[-I include_dir] add a directory to included directories");

// TODO: should save token id in the token

int main(int argc, char* argv[])
{
    bool run = false;
    Platform platform = Platform::gnu_asm_86_64;
    programName = shift_args(&argc, &argv);
    auto args = FLAG_BASE::parse_flags(argc, argv);

    if (argc == 0) {
        mlog::error("PROGRAM: ", "incorrect usage");
        mlog::error("PROGRAM: ", "correct usage is");
        print_help_message();
        exit(1);
    }

    if (help_flag1.exists || help_flag2.exists) {
        print_help_message();
        exit(1);
    }
    if (run_flag.exists) {
        run = *run_flag.value;
    }
    if (output_flag.exists) {
        output_path = *output_flag.value;
    }
    if (platform_flag.exists) {
        if (*platform_flag.value == "list") {
            print_platforms();
            exit(0);
        } else if (PLATFORMS.contains(*platform_flag.value)) {
            platform = PLATFORMS.at(*platform_flag.value);
        } else {
            mlog::error("PROGRAM: ", "platform wasn't found");
            exit(1);
        }
    }
    if (include_flag.exists) {
        for (auto& path : *include_flag.value) {
            ctx.includePaths.push_back(path);
        }
    }
    
    std::string inputFile = std::string(shift_args(args));

    std::filesystem::path inputFilePath(inputFile);
    input_no_extention = inputFilePath.stem().string();
    input_path = inputFilePath.parent_path().string();
    #ifdef WIN32
    build_path = input_path+"\\.build";
    std::string output_path = build_path+"\\output";
    #else
    build_path = input_path+"/.build";
    output_path = build_path+"/output";
    #endif
    // Should add the libmlang path to here
    ctx.includePaths.push_back(".");

    #ifdef WIN32
    ctx.includePaths.push_back("D:\\ahmed\\dev\\cpp\\mlc\\test");
    ctx.includePaths.push_back("D:\\ahmed\\dev\\cpp\\mlc\\mlang-std");
    #else 
    ctx.includePaths.push_back("/home/ahmed/dev/cpp/mlc/test");
    ctx.includePaths.push_back("/home/ahmed/dev/cpp/mlc/mlang-std");
    #endif

    if (!std::filesystem::exists(build_path)) {
        if (std::filesystem::create_directory(build_path)) {
            println("Directory created: {}", build_path);
        } else {
            println(stderr, "Failed to create directory: {}", build_path);
        }
        std::ofstream gitignore(f("{}/.gitignore", build_path));
        println("created: {}/.gitignore", build_path);
        gitignore << "*";
        gitignore.close();
    } else {
        println("Directory already exists: {}", build_path);
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

            std::string link_flags;

            // add search paths
            for (auto& path : ctx.search_paths) {
                if (path != "")
                    link_flags += " -L" + (std::string)path;
            }

            // add libraries
            for (auto& lib : ctx.libs) {
                if (lib != "")
                    link_flags += " -l" + (std::string)lib;
            }

            cmd("{} -g -x assembler {}/{}.s -o {}{} {}", CC, build_path, input_no_extention, output_path, EXECUTABLE_EXTENSION, link_flags);
        }break;
    }



    
    if (run)
        return cmd("{}{}", output_path, EXECUTABLE_EXTENSION);
}
