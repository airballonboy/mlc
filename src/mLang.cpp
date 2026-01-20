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
#include "tools/utils.h"
#include "lexar.h"


std::string programName;
fs::path output_path;

void print_platforms() {
    std::println("platforms");
    for (auto plat : PLATFORMS) {
        std::println("  {:8}", plat.first);
    }
}
void print_help_message() {
    mlog::log("Usage: ", mlog::Blue, f("{} input.mlang [options]", programName).c_str());
    std::println("\noptions:");
    for (auto& flag: FLAG_BASE::flags) {
        std::println("  {}", flag->desc);
    }
}

Flag<bool>                     help_flag     ({"-h", "--help"}, "[-h] [--help]    prints this message");
Flag<bool>                     run_flag      ("-run"          , "[-run]           runs the program after compilation");
Flag<std::string>              output_flag   ("-o"            , "[-o executable]  outputs the program into the name provided");
Flag<std::string>              platform_flag ("-t"            , "[-t platform]    chooses which platform to compile to");
Flag<std::vector<std::string>> include_flag  ("-I"            , "[-I include_dir] add a directory to included directories");

// TODO: should save token id in the token

int main(int argc, char* argv[])
{
    bool run = false;
    Platform platform = Platform::gnu_asm_86_64;
    programName = shift_args(&argc, &argv);
    auto args = FLAG_BASE::parse_flags(argc, argv);

    if (help_flag.exists) {
        print_help_message();
        exit(1);
    }

    if (argc == 0) {
        mlog::error("PROGRAM: ", "incorrect usage");
        mlog::error("PROGRAM: ", "correct usage is");
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
    
    if (!inputFile.ends_with(".mlang")) {
        mlog::error("unknown file path or extension was provided");
        exit(1);
    }

    std::filesystem::path inputFilePath(inputFile);
    input_no_extension = inputFilePath.stem();
    input_path = inputFilePath.parent_path();
    build_path = input_path/".build";
    output_path = build_path/"output";

    ctx.includePaths.push_back(".");
    ctx.includePaths.push_back(MSTD_PATH);
    ctx.includePaths.push_back(PROJECT_PATH"/test");

    if (!std::filesystem::exists(build_path)) {
        if (std::filesystem::create_directory(build_path)) {
            std::println("Directory created: {}", build_path.string());
        } else {
            std::println(stderr, "Failed to create directory: {}", build_path.string());
        }
        std::ofstream gitignore(build_path/".gitignore");
        std::println("created: {}", (build_path/".gitignore").string());
        gitignore << "*";
        gitignore.close();
    } else {
        std::println("Directory already exists: {}", build_path.string());
    }


    std::string file_data{};
    // should add the functions somehow else
    file_data += "func string___len(string* this)->int{int len=0; while(**this){*this+=1;len+=1;}return len;}";
    file_data += readFileToString(inputFile);
	auto lexar = Lexar(file_data, inputFile);

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

            int ret = cmd(CC" -g -x assembler {}.s -o {}{} {}", (build_path/input_no_extension).string(), output_path.string(), EXECUTABLE_EXTENSION, link_flags);
            if (ret != 0) {
                mlog::error("Program aborted");
                exit(ret);
            }
        }break;
    }



    
    if (run)
        return cmd("{}{}", output_path.string(), EXECUTABLE_EXTENSION);
}
