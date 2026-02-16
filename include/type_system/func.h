#pragma once
#include <string>
#include <vector>
#include "type_system/type_info.h"
#include "type_system/kind.h"
#include "type_system/variable.h"
#include "instruction.h"


class Func {
public:
    TypeInfo return_type = type_infos.at("void");
    Kind     return_kind{};

    int arguments_count = 0;
    VariableStorage arguments;

    std::string name{};
    
    VariableStorage local_variables{};
    size_t stack_size = 0;

    bool external = false;
    bool variadic = false;
    bool c_variadic = false;
    bool is_static = false;
    std::string link_name{};
    std::string lib{};
    std::string search_path{};

    std::vector<Instruction> body;
};
typedef std::vector<Func> FunctionStorage;
