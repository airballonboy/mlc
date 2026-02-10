#pragma once
#include <vector>
#include <variant>
#include <string>
#include "type_system/variable.h"
#include "operations.h"

struct Func;
using Arg = std::variant<std::string, Variable, VariableStorage, Func>;
struct Instruction {
    Op op;
    std::vector<Arg> args;
};
