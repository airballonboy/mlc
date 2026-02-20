#include "type_system/struct.h"
#include "tools/logger.h"

Struct& Struct::get_from_name(std::string& name, StructStorage& storage) {
    for (auto& strct_ : storage) {
        if (strct_.name == name) return strct_;
    }
    mlog::println("undefined reference to {}", name);
    exit(1);
}
