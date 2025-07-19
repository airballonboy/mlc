#include "codegen/ir.h"
#include "types.h"
#include "tools/format.h"
#include <any>
#include <cstdint>
#include <cstdio>
#include <print>
#include <string>

void ir::compileProgram() {
    if (m_program == nullptr) return;
    output.append(f("\n"));
    for (auto& func : m_program->func_storage) {
        compileFunction(func);
    }
    std::print("{}", output);
}
void ir::compileFunction(Func func) {
    output.append(f("{}:\n", func.name));
    for (auto& inst : func.body) {
        switch (inst.op) {
            case Op::RETURN: {
                if (std::any_cast<int>(inst.args[0]) == (int)Type::Int32_t)
                    output.append(f("   ret({})\n", std::any_cast<int32_t>(inst.args[1])));
            }break;
            case Op::LOAD_CONST: {
                output.append(f("   load({})\n", std::any_cast<int32_t>(inst.args[0])));
            }break;
        }
    }
}
