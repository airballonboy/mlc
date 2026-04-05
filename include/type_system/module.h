#pragma once 
#include <string>
#include <unordered_map>
#include <type_system/variable.h>


class Func;
typedef std::vector<Func> FunctionStorage;
class Module {
public:
    std::string name{};
    std::unordered_map<std::string, Module> module_storage;
    FunctionStorage func_storage;
    VariableStorage var_storage;
};
typedef std::unordered_map<std::string, Module> ModuleStorage;
