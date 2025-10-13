#include "codegen/ir.h"
#include "types.h"
#include "tools/format.h"
#include <any>
#include <cstdint>
#include <cstdio>
#include <print>
#include <string>
#include <fstream>

void ir::compileProgram() {
    if (m_program == nullptr) return;
    for (auto& func : m_program->func_storage) {
        compileFunction(func);
    }
    std::ofstream outfile(f("{}/{}.ir", build_path, input_no_extention));
    outfile << output;
    outfile.close();
}
void ir::compileFunction(Func func) {
    output.append(f("{}:\n", func.name));
    for (auto& inst : func.body) {
        switch (inst.op) {
            case Op::RETURN: {
                if (std::any_cast<int>(inst.args[0]) == (int)Type::Int32_t)
                    output.appendf("    ret({})\n", std::any_cast<int32_t>(inst.args[1]));
            }break;
            case Op::CALL: {
                std::string func_name = std::any_cast<std::string>(inst.args[0]);
                VariableStorage args  = std::any_cast<VariableStorage>(inst.args[1]);
                output.appendf("    {}(", func_name);
                for (size_t i = 0; i < args.size(); i++) {
                    output.appendf("$({})", args[i].name);
                    if (i+1 < args.size())
                        output.appendf(", ");
                }
                output.appendf(")\n");

            }break;
        }
    }
}
