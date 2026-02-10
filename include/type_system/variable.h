#pragma once
#include "type_system/type_info.h"
#include "type_system/kind.h"
#include <any>
#include <vector>

class Variable;
typedef std::vector<Variable> VariableStorage;

struct Variable {
    TypeInfo    type_info = type_infos.at("void");
    std::string name{};
    std::any    value{};
    int64_t     deref_count = 0;
    size_t      offset = 0;
    size_t      size = 0;
    Variable* parent = nullptr;
    VariableStorage members{};
    Kind kind{};
};

