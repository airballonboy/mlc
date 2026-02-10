#pragma once
#include <vector>
#include <variant>
#include <string>
#include "type_system/variable.h"
#include "operations.h"

class Func;
using Arg = std::variant<std::string, Variable, VariableStorage, Func>;
class Instruction {
public:
    Op op;
    std::vector<Arg> args;
};
