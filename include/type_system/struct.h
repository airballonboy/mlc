#pragma once
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include "type_system/variable.h"

class Struct;
typedef std::vector<Struct> StructStorage;

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
