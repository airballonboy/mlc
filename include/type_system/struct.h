#pragma once
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

class Struct;
typedef std::vector<Struct> StructStorage;
class Variable;
typedef std::vector<Variable> VariableStorage;

class Struct {
public:
    static Struct& get_from_name(std::string& name, StructStorage& storage);
public:
    size_t id = 0;
    std::string name{};
    VariableStorage var_storage;
    std::unordered_map<int, Variable> defaults;
    size_t size;
    size_t alignment;
    bool is_float_only = false;
};
