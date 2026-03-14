#pragma once
#include <vector>
#include <variant>
#include <string>
#include "type_system/variable.h"
#include "operations.h"

class Func;
using Arg = std::variant<std::string, BinOp, Variable, VariableStorage, Func>;
struct Instruction {
    Op op;
    std::vector<Arg> args;
};
