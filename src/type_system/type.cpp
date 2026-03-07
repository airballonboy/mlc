#include "type_system/type.h"
#include "type_system/struct.h"
#include "type_system/variable.h"
Type::Type(TypeInfo i, uint32_t q) 
    : info(i),
      qualifiers(q)
{
    if (i.kind == Kind::Pointer) {
        ptr_data = std::make_unique<PtrData>();
        ptr_data->pointee = std::make_unique<Type>();
    } else if (i.kind == Kind::Func) {
        func_data = std::make_unique<FuncData>();
        func_data->return_type = std::make_unique<Type>();
    } else if (i.kind == Kind::Struct) {
        struct_data = std::make_unique<Struct>();
    }
}
Type::Type(Kind k, uint32_t q) 
    : info({.kind = k}),
      qualifiers(q)
{
    if (k == Kind::Pointer) {
        this->ptr_data = std::make_unique<PtrData>();
        this->ptr_data->pointee = std::make_unique<Type>();
    } else if (k == Kind::Func) {
        this->func_data = std::make_unique<FuncData>();
        this->func_data->return_type = std::make_unique<Type>();
    } else if (k == Kind::Struct) {
        this->struct_data = std::make_unique<Struct>();
    }
}
Type::Type(const Type& other)
    : info(other.info),
      qualifiers(other.qualifiers)
{
    if (info.kind == Kind::Pointer && other.ptr_data) {
        ptr_data = std::make_unique<PtrData>();
        ptr_data->pointee = std::make_unique<Type>(*other.ptr_data->pointee);
    }
    else if (info.kind == Kind::Func && other.func_data) {
        func_data = std::make_unique<FuncData>();
        func_data->return_type = std::make_unique<Type>(*other.func_data->return_type);
        for (const auto& t : other.func_data->args)
            func_data->args.push_back(std::make_unique<Type>(*t));
    }
    else if (info.kind == Kind::Struct && other.struct_data) {
        struct_data = std::make_unique<Struct>(*other.struct_data);
    }
}
// swap helper
static inline void swap_type(Type& a, Type& b) noexcept {
    using std::swap;
    swap(a.info, b.info);
    swap(a.qualifiers, b.qualifiers);
    swap(a.struct_data, b.struct_data);
    swap(a.ptr_data, b.ptr_data);
    swap(a.func_data, b.func_data);
}

// copy-and-swap assignment
Type& Type::operator=(Type other) {
    swap_type(*this, other);
    return *this;
}
Type::~Type() = default;
Type get_base_type(Type t) {
    while (t.info.kind == Kind::Pointer) {
        t = *t.ptr_data->pointee;
    }
    return t;
}
Type set_ptr_count(Type base, size_t count) {
    Type ptr = base;
    while (count-- > 0) {
        auto old_ptr = ptr;
        ptr = Type(Kind::Pointer);
        *ptr.ptr_data->pointee = old_ptr;
    }
    return ptr;
}
size_t get_ptr_count(Type t) {
    size_t i = 0;
    while (t.info.kind == Kind::Pointer) {
        i++;
        t = *t.ptr_data->pointee;
    }
    return i;
}
Type make_ptr(Type base) {
    Type ptr = Type(Kind::Pointer);
    *ptr.ptr_data->pointee = base;
    return ptr;
}

size_t get_typeid(Type t) {
    return t.info.id;
}
size_t get_typeid(TypeInfo t) {
    return t.id;
}
size_t get_typeid(std::string t) {
    if (!type_infos.contains(t)) {
        TODO(mlog::format("cannot find type {}", t));
    }
    return type_infos.at(t).id;
}