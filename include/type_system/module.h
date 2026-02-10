#pragma once 
#include <string>
#include <unordered_map>
#include <type_system/func.h>
#include <type_system/variable.h>


struct Module {
    std::string name{};
    std::unordered_map<std::string, Module> module_storage;
    FunctionStorage func_storage;
    VariableStorage var_storage;
};
typedef std::unordered_map<std::string, Module> ModuleStorage;
