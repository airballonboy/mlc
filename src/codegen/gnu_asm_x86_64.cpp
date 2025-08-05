#include "codegen/gnu_asm_x86_64.h"
#include "tools/logger.h"
#include "types.h"
#include "tools/format.h"
#include <any>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <print>
#include <string>
#include <string_view>

void gnu_asm::compileProgram() {
    if (m_program == nullptr) return;
    output.append(".section .text\n");
    for (const auto& func : m_program->func_storage) {
        compileFunction(func);
    }
    for (const auto& var : m_program->var_storage) {
        if (var.type == Type::String_lit)
            output.appendf("{}: .string \"{}\" \n", var.name, std::any_cast<std::string>(var.value));
    }


    std::ofstream outfile(f("{}/{}.s", build_path, input_no_extention));
    outfile << output;
    outfile.close();
}
void gnu_asm::compileFunction(Func func) {
    output.appendf(".global {}\n", func.name);
    output.appendf("{}:\n", func.name);
    output.appendf("    pushq %rbp\n");
    output.appendf("    movq %rsp, %rbp\n");


    for (auto& inst : func.body) {
        switch (inst.op) {
            case Op::RETURN: {
                // NOTE: on Unix it takes the % of the return and 255 so the largest you can have is 255 and then it returns to 0
                Variable arg = std::any_cast<Variable>(inst.args[0]);
                if (arg.type == Type::String_lit)
                    output.appendf("    movq ${}, %rax\n", arg.name);
                else if (arg.type == Type::Int_lit)
                    output.appendf("    movq ${}, %rax\n", std::any_cast<int>(arg.value));
                else if (arg.type == Type::Void_t)
                    output.appendf("    movq $0, %rax\n");
                else 
                    output.appendf("    movq -{}(%rbp), %rax\n", arg.offset);
                //returns zero in main function
                output.appendf("    movq %rbp, %rsp\n");
                output.appendf("    popq %rbp\n");
                output.appendf("    ret\n");
            }break;
            case Op::LOAD_CONST: {
                output.appendf("    load({})\n", std::any_cast<int32_t>(inst.args[0]));
            }break;
            case Op::STORE_VAR: {
                Variable var  = std::any_cast<Variable>(inst.args[0]);
                output.appendf("    movq ${}, -{}(%rbp)\n", TypeToString.at(var.type)(var.value), var.offset);
            }break;
            case Op::CALL: {
                std::string func_name = std::any_cast<std::string>(inst.args[0]);
                VariableStorage args  = std::any_cast<VariableStorage>(inst.args[1]);
                std::string_view arg_register[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
                for (size_t i = 0; i < args.size() && i < std::size(arg_register); i++) {
                    if (args[i].type == Type::String_lit)
                        output.appendf("    movq ${}, %{}\n", args[i].name, arg_register[i]);
                    else if (args[i].type == Type::Int_lit)
                        output.appendf("    movq ${}, %{}\n", std::any_cast<int>(args[i].value), arg_register[i]);
                    else 
                        output.appendf("    movq -{}(%rbp), %{}\n", args[i].offset, arg_register[i]);


                }
                output.appendf("    call {}\n", func_name);
            }break;
        }
    }
}
