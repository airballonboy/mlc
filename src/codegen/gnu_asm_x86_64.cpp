#include "codegen/gnu_asm_x86_64.h"
#include "types.h"
#include "tools/format.h"
#include <any>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <print>
#include <string>

void gnu_asm::compileProgram() {
    if (m_program == nullptr) return;
    output.append(".section .text\n");
    for (auto& func : m_program->func_storage) {
        compileFunction(func);
    }
    std::ofstream outfile(f("{}/{}.as", build_path, input_no_extention));
    outfile << output;
    outfile.close();
    //std::print("{}", output);
}
void gnu_asm::compileFunction(Func func) {
    output.append(f(".global {}\n", func.name));
    output.append(f("{}:\n", func.name));
    output.append("    pushq %rbp\n");
    output.append("    movq %rsp, %rbp\n");


    for (auto& inst : func.body) {
        switch (inst.op) {
            case Op::RETURN: {
                output.append("    movq %rbp, %rsp\n");
                output.append("    popq %rbp\n");
                if (std::any_cast<int>(inst.args[0]) == (int)Type::Int32_t) {
                    int32_t arg = std::any_cast<int32_t>(inst.args[1]);
                    output.append(f("    movq ${}, %rax\n", arg));
                    output.append("    ret\n");
                }
            }break;
            case Op::LOAD_CONST: {
                output.append(f("   load({})\n", std::any_cast<int32_t>(inst.args[0])));
            }break;
        }
    }
}
