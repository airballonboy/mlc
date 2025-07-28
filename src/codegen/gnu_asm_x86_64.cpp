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
    std::ofstream outfile(f("{}/{}.s", build_path, input_no_extention));

    // TODO: implement actual print
    output.appendf(".global print\n")
          .appendf("print:\n")
          .appendf("    movq $hello_str, %rdi\n")  
          .appendf("    call puts\n")        
          .appendf("    ret\n")
          .appendf("hello_str: .string \"Hello from mlang!\\n\" \n");

    outfile << output;
    outfile.close();
    //std::print("{}", output);
}
void gnu_asm::compileFunction(Func func) {
    output.appendf(".global {}\n", func.name);
    output.appendf("{}:\n", func.name);
    output.appendf("    pushq %rbp\n");
    output.appendf("    movq %rsp, %rbp\n");


    for (auto& inst : func.body) {
        switch (inst.op) {
            case Op::RETURN: {
                output.appendf("    movq %rbp, %rsp\n");
                output.appendf("    popq %rbp\n");
                switch (std::any_cast<int>(inst.args[0])) {
                    case (int)Type::Int32_t: {
                        // NOTE: on Unix it takes the % of the return and 255 so the largest you can have is 255 and then it returns to 0
                        int32_t arg = std::any_cast<int32_t>(inst.args[1]);
                        output.appendf("    movq ${}, %rax\n", arg);
                        output.appendf("    ret\n");
                    }break;
                    case (int)Type::Void_t: {
                        //returns zero in main function
                        if (func.name == "main")
                            output.appendf("    movq $0, %rax\n");
                        output.appendf("    ret\n");
                    }break;
                }
            }break;
            case Op::LOAD_CONST: {
                output.appendf("    load({})\n", std::any_cast<int32_t>(inst.args[0]));
            }break;
            case Op::CALL: {
                std::string func_name = std::any_cast<std::string>(inst.args[0]);
                output.appendf("    call {}\n", func_name);
            }break;
        }
    }
}
