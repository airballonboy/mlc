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
#include "tools/file.h"
#include "codegen/ir.h"
#include "tools/logger.h"
#include "tools/format.h"
#include "tools/utils.h"
#include "lexar.h"


std::string programName;
fs::path output_path;

void print_platforms() {
    mlog::log(mlog::Cyan, "Platforms:");
    for (auto plat : PLATFORMS) {
        std::println("  {:8}", plat.first);
    }
}
void print_targets() {
    mlog::log(mlog::Cyan, "Targets:");
    for (auto targ : TARGETS) {
        std::println("  {:8}", targ.first);
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
Flag<std::string>              output_flag   ("-o"            , "[-o output]      outputs the program into the name provided");
Flag<std::string>              platform_flag ("-p"            , "[-p platform]    chooses which platform to compile to, can use `-p list` to list platforms");
Flag<std::string>              target_flag   ("-t"            , "[-t target]      chooses which target to compile to, can use `-t list` to list targets");
Flag<std::vector<std::string>> include_flag  ("-I"            , "[-I include_dir] add a directory to included directories");

int main(int argc, char* argv[])
{
    bool run = false;
    programName = shift_args(&argc, &argv);
    auto args = FLAG_BASE::parse_flags(argc, argv);

    // Setting default platform and target for operating system
#if defined(WIN32)
    Platform  platform = Platform::Windows;
    BuildTarget target = BuildTarget::gnu_asm_86_64;
#else
    Platform  platform = Platform::Linux;
    BuildTarget target = BuildTarget::gnu_asm_86_64;
#endif

    // Parse flags
    if (help_flag.exists) {
        print_help_message();
        exit(0);
    }
    if (platform_flag.exists) {
        if (*platform_flag.value == "list") {
            print_platforms();
            exit(0);
        } else if (PLATFORMS.contains(*platform_flag.value)) {
            platform = PLATFORMS.at(*platform_flag.value);
        } else {
            mlog::error("PROGRAM: ", "platform wasn't found");
            mlog::error("Program aborted");
            exit(1);
        }
    }
    if (target_flag.exists) {
        if (*target_flag.value == "list") {
            print_targets();
            exit(0);
        } else if (TARGETS.contains(*target_flag.value)) {
            target = TARGETS.at(*target_flag.value);
        } else {
            mlog::error("PROGRAM: ", "target wasn't found");
            mlog::error("Program aborted");
            exit(1);
        }
    }
    if (run_flag.exists) {
        run = *run_flag.value;
    }
    if (output_flag.exists) {
        output_path = *output_flag.value;
    }
    if (include_flag.exists) {
        for (auto& path : *include_flag.value) {
            ctx.includePaths.push_back(path);
        }
    }

    if (argc == 0) {
        mlog::error("PROGRAM: ", "incorrect usage");
        mlog::error("PROGRAM: ", "correct usage is");
        print_help_message();
        mlog::error("Program aborted");
        exit(1);
    }
    
    std::string input_file_name = std::string(shift_args(args));
    
    if (!input_file_name.ends_with(".mlang")) {
        mlog::error("unknown file path or extension was provided");
        mlog::error("Program aborted");
        exit(1);
    }

    // Configuring paths
    input_file = fs::path(input_file_name);
    input_path = input_file.parent_path();
    build_path = input_path/".build";
    if (!output_flag.exists)
        output_path = build_path/"output";

    // Setting default included paths
    ctx.includePaths.push_back(".");
    ctx.includePaths.push_back(MSTD_PATH);
    ctx.includePaths.push_back(PROJECT_PATH"/test");

    // Make build directory and add .gitignore to it
    if (!fs::exists(build_path)) {
        if (fs::create_directory(build_path)) {
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


    // First compilation step: tokenizing the input file
    auto builtin_lexar = add_builtin_file("internal/builtins.mlang");
	auto lexar = Lexar(readFileToString(input_file_name), input_file_name);
    lexar.pushTokensAt(0, &builtin_lexar);

    // Second compilation step: parsing the already tokenized file
    Parser parser(&lexar);
    auto prog = parser.parse();

    // Setting the default platform
    prog->platform = platform;

    // Final compilation step: generating assembly, assembling and linking
    switch (target) {
        case BuildTarget::ir: {
            ir compiler_ir(prog);
            compiler_ir.compileProgram();
        }break;
        case BuildTarget::gnu_asm_86_64: {
            gnu_asm compiler(prog);
            compiler.compileProgram();

            std::string link_flags;

            // Add search paths
            for (auto& path : ctx.search_paths) {
                if (path != "")
                    link_flags += " -L" + (std::string)path;
            }

            // Add libraries
            for (auto& lib : ctx.libs) {
                if (lib != "")
                    link_flags += " -l" + (std::string)lib;
            }

            std::string output_name;
            if (platform == Platform::Windows)
                output_name = output_path.string()+".exe";
            if (platform == Platform::Linux)
                output_name = output_path.string();
            int ret = cmd(CC" -g -x assembler {}.s -o {} {}", (build_path/input_file.stem()).string(), output_name, link_flags);
            if (ret != 0) {
                mlog::error("Program aborted");
                exit(ret);
            }
        }break;
    }



    
    if (run) {
        if (platform == Platform::Windows)
            return cmd("{}.exe", output_path.string());
        if (platform == Platform::Linux)
            return cmd("{}", output_path.string());
    }
}
