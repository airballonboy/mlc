#pragma once
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

class Struct {
public:
    size_t id = 0;
    std::string name{};
    VariableStorage var_storage;
    std::unordered_map<int, Variable> defaults;
    size_t size;
    size_t alignment;
    bool is_float_only = false;
};
typedef std::vector<Struct> StructStorage;
