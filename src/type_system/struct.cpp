#include "type_system/struct.h"
#include "tools/logger.h"
#include "type_system/variable.h"

Struct& Struct::get_from_name(std::string& name, StructStorage& storage) {
    for (auto& strct_ : storage) {
        if (strct_.name == name) return strct_;
    }
    mlog::println(stderr, "undefined reference to struct {}", name);
    exit(1);
}
size_t Struct::get_element_iterator(std::string& name) {
    size_t i = 0;
    for (auto v : this->var_storage) {
        if (v.name == name) break;
        i++;
    }
    return i;
}
