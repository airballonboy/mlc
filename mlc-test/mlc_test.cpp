#include "tools/utils.h"
#include <string>
#include <array>
#include <print>
#include <tuple>
#include <vector>
using std::vector;
using std::tuple;
using std::string;
using std::print;
using std::println;

vector<tuple<string, string>> tests_and_outputs = {
    {MTEST_PATH"function_return.mlang", "12\n"},
    {MTEST_PATH"binary_operations.mlang", "18\n6\n72\n2\n"},
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
    for (auto& [test, output] : tests_and_outputs) {
        string output_file = test;
        output_file.erase(output_file.size() - 6);
#ifdef WIN32
        auto [build_status, _] = cmd_with_output("{}\\Release\\mlc {} -o {}", OUTPUT_PATH, test, output_file);
#else
        auto [build_status, _] = cmd_with_output("{}mlc {} -o {}", OUTPUT_PATH, test, output_file);
#endif
        if (build_status != 0) number_of_faild_builds += 1;
        
        auto [test_status, test_output] = cmd_with_output("{}", output_file);
        bool test_resault = (test_status == 0) && (test_output == output);
        if (!test_resault) number_of_faild_tests += 1;
        string pretty_test_name = remove_substr(test, "mlc-test/");
        print("{:30} | ", pretty_test_name);
        print("{:^12} | ", build_status == 0 ? "[OK]" : "[FAIL]");
        print("{:^12}\n", test_resault ? "[OK]" : "[FAIL]");
    }
    println("-------------------------------------------------------------");
    println("Number of Builds failed {}", number_of_faild_builds);
    println("Number of Tests failed  {}", number_of_faild_tests);
    if (number_of_faild_builds > 0 || number_of_faild_tests > 0)
        return 1;
    return 0;
}
