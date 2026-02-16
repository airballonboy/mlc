#include "type_system/func.h"
#include "tools/logger.h"
#include "type_system/module.h"
#include "program.h"
#include <print>

bool Func::is_in_storage(std::string_view name, const FunctionStorage& storage) {
    for (auto& func : storage) {
        if (func.name == name) return true;
    }
    return false;
}
Func& Func::get_from_name(std::string_view name, FunctionStorage& storage) {
    if (!is_in_storage(name, storage)) {std::println("{}", name); TODO("func doesn't exist");}

    for (auto& func : storage) {
        if (func.name == name) return func;
    }
    std::println("undefined reference to {}", name);
    exit(1);
}
Func Func::get_from_module(std::string name, Module mod) {
    for (const auto& func : mod.func_storage) {
        if (name == func.name) return func;
    }
    for (auto [prefix, module] : mod.module_storage) {
        if (name.starts_with(prefix))
            return get_from_module(name.erase(0, prefix.size()), mod);
    }
    std::println("undefined reference to {}", name);
    exit(1);
}
Func Func::get_from_program(std::string name, Program prog) {
    for (auto func : prog.func_storage) 
        if (func.name == name) 
            return func;
    for (auto [prefix, mod] : prog.module_storage) {
        if (name.starts_with(prefix))
            return get_from_module(name.erase(0, prefix.size()), mod);
    }
    std::println("undefined reference to {}", name);
    exit(1);
}
