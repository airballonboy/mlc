#include "type_system/struct.h"
#include <print>

Struct& Struct::get_from_name(std::string& name, StructStorage& storage) {
    for (auto& strct_ : storage) {
        if (strct_.name == name) return strct_;
    }
    std::println("undefined reference to {}", name);
    exit(1);
}
