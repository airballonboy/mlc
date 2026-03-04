#include "type_system/type.h"
#include "type_system/struct.h"
Type::Type(TypeInfo i, uint32_t q) : info(i), qualifiers(q) {
    if (i.kind == Kind::Pointer) {
        ptr_data = new PtrData;
        ptr_data->pointee = new Type;
    } else if (i.kind == Kind::Func) {
        func_data = new FuncData;
        func_data->return_type = new Type;
    } else if (i.kind == Kind::Struct) {
        struct_data = new Struct;
    }
}
Type::Type(Kind k, uint32_t q) : info({.kind = k}), qualifiers(q) {
    if (k == Kind::Pointer) {
        this->ptr_data = new PtrData;
        this->ptr_data->pointee = new Type;
    } else if (k == Kind::Func) {
        this->func_data = new FuncData;
        this->func_data->return_type = new Type;
    } else if (k == Kind::Struct) {
        this->struct_data = new Struct;
    }
}
Type::Type(const Type& other)
    : info(other.info),
      qualifiers(other.qualifiers),
      struct_data(nullptr),
      ptr_data(nullptr),
      func_data(nullptr)
{
    if (info.kind == Kind::Pointer && other.ptr_data) {
        ptr_data = new PtrData;
        ptr_data->pointee = new Type(*other.ptr_data->pointee);
    }
    else if (info.kind == Kind::Func && other.func_data) {
        func_data = new FuncData;
        func_data->return_type = new Type(*other.func_data->return_type);
        for (auto t : other.func_data->args)
            func_data->args.push_back(new Type(*t));
    }
    else if (info.kind == Kind::Struct && other.struct_data) {
        struct_data = new Struct(*other.struct_data);
    }
}
Type& Type::operator=(const Type& other) {
    if (this == &other) return *this;
    this->~Type();

    new (this) Type(other);
    return *this;
}
Type::~Type() {
    if (this->info.kind == Kind::Pointer) {
        delete ptr_data->pointee;
        delete ptr_data;
    } else if (this->info.kind == Kind::Func) {
        delete func_data->return_type;
        for (auto t : func_data->args)
            delete t;
        delete func_data;
    } else if (this->info.kind == Kind::Struct) {
        delete struct_data;
    }
}
