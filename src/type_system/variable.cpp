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
    mlog::println("undefined reference to {}", name);
    exit(1);
}
size_t Variable::size_in_bytes(Type t) {
    switch (t) {
        case Type::Bool_t:   return 1; break;
        case Type::Char_t:   return 1; break;
        case Type::Int8_t:   return 1; break;
        case Type::Int16_t:  return 2; break;
        case Type::Int32_t:  return 4; break;
        case Type::Int64_t:  return 8; break;
        case Type::Typeid_t: return 8; break;
        case Type::Ptr_t:    return 8; break;
        case Type::Size_t:   return 8; break;
        case Type::Float_t:  return 4; break;
        case Type::Double_t: return 8; break;

        case Type::String_t: return 8; break;
        case Type::Void_t:   return 0; break;

        default: 
            TODO(mlog::format("type {} doesn't have default size", (int)t));
    }
    return 0;
}
std::any Variable::default_value(Type t) {
    switch (t) {
        case Type::Size_t:
        case Type::Int8_t:
        case Type::Int16_t:
        case Type::Int32_t:
        case Type::Char_t:
        case Type::Bool_t:
        case Type::Typeid_t:
        case Type::Int64_t: return (int64_t)0; break;
        case Type::Double_t:
        case Type::Float_t: return (double)0.0; break;

        case Type::String_t: return ""   ; break;
        case Type::Void_t:   return (int64_t)0    ; break;

        default: 
            TODO(mlog::format("type {} doesn't have default", (int)t));
    }
    return 0;
}
