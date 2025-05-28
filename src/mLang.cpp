#include "dataTypes.hpp"
#include "fileOperations.hpp"
#include "tokenizer.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <variant>
#include <charconv>
#include <string>


char* shift_args(int *argc, char ***argv) {
	assert("no more args" && *argc > 0);
    char *result = (**argv);
    (*argv) += 1;
    (*argc) -= 1;
    return result;
}

using VariantValue = std::variant<int, bool, float, char, std::string>;

VariantValue getVariableValue(ML::Variable v) {
    switch (v.type) {
		case ML::INT:    return *static_cast<int*>(v.value);
        case ML::BOOL:   return *static_cast<bool*>(v.value);
        case ML::FLOAT:  return *static_cast<float*>(v.value);
        case ML::CHAR:   return *static_cast<char*>(v.value);
        case ML::STRING: return *static_cast<std::string*>(v.value);
        default:     throw std::invalid_argument("Unknown type");
    }
}


int main(int argc, char** argv) 
{
	std::string programName = shift_args(&argc, &argv);
	ML::File current = ML::openFile(shift_args(&argc, &argv));
	ML::Tokens ts = ML::fileToTokens(current);
	int i = 9;
	ML::Variable var = { };
	for (size_t i = 0; i < ts.size();i++){
		auto& t = ts[i];
		if (t.data == "int"){
			var.type = ML::INT;
			var.label = {ts[i + 1].data};
			if (ts[i + 2].data == "="){
				int val;
				std::from_chars(ts[i + 3].data.data(), ts[i + 3].data.data() + ts[i + 3].data.size(), val);
				var.value = &val;

			}
			
		}
	}
	std::visit([](auto&& val) {
        std::cout << "Value: " << val << "\n";
    }, getVariableValue(var));
}
