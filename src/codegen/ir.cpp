#include "codegen/ir.h"
#include "types.h"
#include <any>
#include <cstdint>
#include <print>

void ir::compileProgram() {
    if (m_program == nullptr) return;
    for (auto& inst : *m_program) {
        switch (inst.op) {
            case Op::RETURN: {
                if (std::any_cast<int>(inst.args[0]) == (int)Type::Int32_t)
                    std::println(stdout, "ret({})", std::any_cast<int32_t>(inst.args[1]));
            }break;
        
        }
    }
}
