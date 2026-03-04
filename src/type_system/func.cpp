#include "type_system/func.h"
#include "tools/logger.h"
#include "type_system/module.h"
#include "program.h"

Func::Func() {
    type = Type(Kind::Func);
}
Func::Func(Type ret_type, std::vector<Type> args) {
    type = Type(Kind::Func);
    *type.func_data->return_type = ret_type;
    for (auto arg : args) {
        Type* new_arg = new Type;
        *new_arg = arg;
        type.func_data->args.push_back(new_arg);
    }
}
bool Func::is_in_storage(std::string_view name, const FunctionStorage& storage) {
    for (auto& func : storage) {
        if (func.name == name) return true;
    }
    return false;
}
Func& Func::get_from_name(std::string_view name, FunctionStorage& storage) {
    if (!is_in_storage(name, storage)) {mlog::println("{}", name); TODO("func doesn't exist");}

    for (auto& func : storage) {
        if (func.name == name) return func;
    }
    mlog::println(stderr, "undefined reference to func {}", name);
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
    mlog::println(stderr, "undefined reference to func {}", name);
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
    mlog::println(stderr, "undefined reference to func {}", name);
    exit(1);
}
