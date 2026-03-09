#pragma once
#include "type_system/type.h"
#include <any>
#include <string_view>
#include <string>
#include <vector>

class Variable;
typedef std::vector<Variable> VariableStorage;

class Variable {
public:
    static bool      is_in_storage(std::string_view name, VariableStorage& storage);
    static Variable& get_from_name(std::string_view name, VariableStorage& storage);
    static size_t    size_in_bytes(size_t id);
    static std::any  default_value(Type t);
public:
    Type        type = {type_infos.at("void")};
    std::string name{};
    int64_t     Int_val;
    double      Double_val;
    std::string String_val;
    int64_t     deref_count = 0;
    size_t      offset = 0;
    size_t      size = 0;
    Variable* parent = nullptr;
    VariableStorage members{};
};
