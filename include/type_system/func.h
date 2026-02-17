#pragma once
#include <string>
#include <vector>
#include "type_system/type_info.h"
#include "type_system/kind.h"
#include "type_system/variable.h"
#include "instruction.h"

class Program;
class Module;
class Func;
typedef std::vector<Func> FunctionStorage;

class Func {
public:
    static bool  is_in_storage(std::string_view name, const FunctionStorage& storage);
    static Func  get_from_program(std::string name, Program prog);
    static Func  get_from_module(std::string name, Module mod);
    static Func& get_from_name(std::string_view name, FunctionStorage& storage);
public:
    TypeInfo return_type = type_infos.at("void");
    Kind     return_kind{};

    int arguments_count = 0;
    VariableStorage arguments;

    std::string name{};
    
    VariableStorage local_variables{};
    size_t stack_size = 0;

    bool external   = false;
    bool variadic   = false;
    bool c_variadic = false;
    bool is_static  = false;
    bool is_used    = false;
    std::string link_name{};
    std::string lib{};
    std::string search_path{};

    std::vector<Instruction> body;
};
