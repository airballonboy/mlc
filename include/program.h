#pragma once
#include "type_system/module.h"
#include "type_system/func.h"
#include "type_system/struct.h"
#include "platform.h"

class Program {
public:
    ModuleStorage   module_storage;
    FunctionStorage func_storage;
    VariableStorage var_storage;
    StructStorage   struct_storage;
    Platform platform;
};
