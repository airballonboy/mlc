#pragma once
#include "type_system/type_info.h"
#include "type_system/kind.h"
#include <any>
#include <string_view>
#include <vector>

class Variable;
typedef std::vector<Variable> VariableStorage;

class Variable {
public:
    static bool      is_in_storage(std::string_view name, VariableStorage& storage);
    static Variable& get_from_name(std::string_view name, VariableStorage& storage);
    static size_t    size_in_bytes(Type t);
    static std::any  default_value(Type t);
public:
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
