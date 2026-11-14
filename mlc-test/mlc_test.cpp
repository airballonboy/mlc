#include "tools/logger.h"
#include "tools/utils.h"
#include <filesystem>
#include <string>
#include <tuple>
#include <vector>
using std::vector;
using std::tuple;
using std::string;
using mlog::println;
using mlog::print;
using mlog::format;
namespace  fs = std::filesystem;

#ifdef WIN32
fs::path mlc = fs::path(OUTPUT_PATH)/"Release"/"mlc";
#else
fs::path mlc = fs::path(OUTPUT_PATH)/"mlc";
#endif

vector<tuple<string, string>> tests_and_outputs = {
    {MTEST_PATH"function_return.mlang", "12\n"},
    {MTEST_PATH"binary_operations.mlang", "18\n6\n72\n2\n"},
    {MTEST_PATH"struct.mlang", "{12, 43, 54}\n422\n"},
    {MTEST_PATH"struct_return.mlang", "0xFF, 0x18, 0x18, 0xFF\n120.0, 120.0, 435.0\n505.0, 123.0, 533.0, 476.0\n"},
};
std::string remove_substr(const string str, const string sub) {
    size_t pos = str.find(sub);
    if (pos != std::string::npos) {
        return str.substr(pos + sub.length());
    }
    return str;
}
int main () {
    println("-------------------------------------------------------------");
    println("{:^30} | {:^12} | {:^12}", "test name", "build status", "test status");
    int number_of_faild_builds = 0;
    int number_of_faild_tests  = 0;
    string total_output{};
    for (auto& [test, output] : tests_and_outputs) {
        string output_file = test;
        // Erase ".mlang" suffix
        output_file.erase(output_file.size() - 6);
        auto [build_status, err] = cmd_with_output("{} {} -o {}", mlc.string(), fs::path(test).string(), fs::path(output_file).string());

        bool test_output_match = true;
        bool test_result = false;
        string&& test_output = "";
        if (build_status == 0) {
            auto [test_status, test_output] = cmd_with_output("{}", output_file);
            test_output_match = (test_output == output);
            test_result = (test_status == 0) && test_output_match;
            // Delete output files after testing them
#if defined(WIN32)
            output_file += ".exe";
#endif
            fs::remove(output_file);
        }
        string pretty_test_name = remove_substr(test, "mlc-test/");
        print("{:30} | " , pretty_test_name);
        print("{:^12} | ", build_status == 0 ? "[OK]" : "[FAIL]");
        print("{:^12}\n" , test_result ? "[OK]" : "[FAIL]");

        if (build_status != 0) {
            total_output += format("[ERROR] build couldn't complete\n");
            total_output += format("  on file: {}:\n", test);
            total_output += format("    error = {}", escape_new_lines(err));
            total_output += format("    exit_code = {}", build_status);
        }

        if (!test_output_match) {
            total_output += format("[ERROR] output mismatch\n");
            total_output += format("  on file: {}:\n", test);
            total_output += format("    expected \"{}\"\n", escape_new_lines(output));
            total_output += format("    but got  \"{}\"\n", escape_new_lines(test_output));
        }

        if (build_status != 0) number_of_faild_builds += 1;
        if (!test_result)      number_of_faild_tests  += 1;
    }
    println("-------------------------------------------------------------");
    println("Number of Builds failed {}", number_of_faild_builds);
    println("Number of Tests failed  {}", number_of_faild_tests);
    print("{}", total_output);
    return number_of_faild_tests + number_of_faild_builds;
}
