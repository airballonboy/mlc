#include "type_system/variable.h"
#include "tools/format.h"
#include "tools/logger.h"
#include "type_system/struct.h"

bool Variable::is_in_storage(std::string_view name, VariableStorage& storage) {
    for (const auto& var : storage) {
        if (var.name == name) return true;
    }
    return false;
}
Variable& Variable::get_from_name(std::string_view name, VariableStorage& storage) {
    for (auto& var : storage) {
        if (var.name == name) return var;
    }
    mlog::println(stderr, "undefined reference to variable {}", name);
    exit(1);
}
size_t Variable::size_in_bytes(size_t id) {
    switch (id) {
        case TypeId::Bool:   return 1; break;
        case TypeId::Char:   return 1; break;
        case TypeId::Int8:   return 1; break;
        case TypeId::Int16:  return 2; break;
        case TypeId::Int32:  return 4; break;
        case TypeId::Int64:  return 8; break;
        case TypeId::Typeid: return 8; break;
        case TypeId::Ptr:    return 8; break;
        case TypeId::USize:   return 8; break;
        case TypeId::Float:  return 4; break;
        case TypeId::Double: return 8; break;

        case TypeId::String: return 8; break;
        case TypeId::Void:   return 0; break;

        default: 
            TODO(mlog::format("type {} doesn't have default size", id));
    }
    return 0;
}
std::any Variable::default_value(Type t) {
    switch (t.info.id) {
        case TypeId::USize:
        case TypeId::Int8:
        case TypeId::Int16:
        case TypeId::Int32:
        case TypeId::Char:
        case TypeId::Bool:
        case TypeId::Typeid:
        case TypeId::Int64: return (int64_t)0; break;
        case TypeId::Double:
        case TypeId::Float: return (double)0.0; break;

        case TypeId::String: return ""   ; break;
        case TypeId::Void:   return (int64_t)0    ; break;

        default: 
            TODO(mlog::format("type {} doesn't have default", t.info.id));
    }
    return 0;
}
