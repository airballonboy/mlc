#include "codegen/gnu_asm_x86_64.h"
#include "context.h"
#include "tools/logger.h"
#include "types.h"
#include "tools/format.h"
#include <any>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <print>
#include <string>
#include <string_view>

int op = 0;
#define MAX_STRING_SIZE 2048
size_t current_string_count = 0;

// TODO: maybe remove the 32 bit stuff
#ifdef WIN32
static std::pair<std::string_view, std::string_view> arg_register[] = {{"%rcx", "ecx"}, {"%rdx", "edx"}, {"%r8", "r8d"}, {"%r9", "r9d"}};
#else	
static std::pair<std::string_view, std::string_view> arg_register[] = {
    {"%rdi", "%edi"}, {"%rsi", "%esi"}, {"%rdx", "%edx"}, {"%rcx", "%ecx"}, {"%r8", "%r8d"}, {"%r9", "%r9d"}
};
#endif

void gnu_asm::compileProgram() {
    if (m_program == nullptr) return;
    output.append(".section .text\n");
    for (const auto& func : m_program->func_storage) {
        if(!func.external) {
            compileFunction(func);
        }
    }
    std::string target = "    movq %rax, %rax";

    size_t pos;
    while ((pos = output.find(target)) != std::string::npos) {
        // erase the line containing it
        size_t lineStart = output.rfind('\n', pos);
        if (lineStart == std::string::npos) lineStart = 0;
        else lineStart++;

        size_t lineEnd = output.find('\n', pos);
        if (lineEnd == std::string::npos) lineEnd = output.size();

        output.erase(lineStart, lineEnd - lineStart + 1);
    }


	output.appendf(".section .rodata\n");
    for (const auto& var : m_program->var_storage) {
        if (var.type == Type::String_lit)
            output.appendf("{}: .string \"{}\" \n", var.name, std::any_cast<std::string>(var.value));
    }
    if (current_string_count != 0) {
        output.appendf(".align 8\n");
        output.appendf(".section .bss\n");
        output.appendf("string_storage: .skip {}\n", current_string_count*MAX_STRING_SIZE);
    }

    output.appendf(".section .text\n");
    output.appendf("// Externals\n");
    for (const auto& func : m_program->func_storage) {
        if (func.external) {
            output.appendf(".global {}\n", func.name);
            output.appendf(".extern {}\n", func.link_name);
            if (func.name != func.link_name)
                output.appendf(".set    {}, {}\n", func.name, func.link_name);
            if (!ctx.libs.contains(func.lib))
                ctx.libs.insert(func.lib);
            if (!ctx.search_paths.contains(func.search_path))
                ctx.search_paths.insert(func.search_path);
        }
    }

    std::ofstream outfile(f("{}/{}.s", build_path, input_no_extention));
    outfile << output;
    outfile.close();
}
void gnu_asm::compileFunction(Func func) {
    // if the function doesn't return you make it return 0
    bool returned = false;

    output.appendf(".global {}\n", func.name);
    output.appendf("{}:\n", func.name);
    output.appendf("    pushq %rbp\n");
    output.appendf("    movq %rsp, %rbp\n");
    func.stack_size = (func.stack_size + 15) & ~15;
    output.appendf("    subq ${}, %rsp\n", func.stack_size);

    for (int i = 0; i < func.arguments_count; i++) {
        if (i < std::size(arg_register)) {
            if (func.arguments[i].type == Type::Int32_t)
                move_reg_to_var(arg_register[i].first, func.arguments[i]);       
            else {
                move_reg_to_reg(arg_register[i].first, "%rax");
                move_reg_to_var("%rax", func.arguments[i]);
            }
        }
    }

    for (auto& inst : func.body) {
        //output.appendf(".op_{}:\n", op++);
        switch (inst.op) {
            case Op::RETURN: {
                // NOTE: on Unix it takes the mod of the return and 256 so the largest you can have is 255 and after it returns to 0
                Variable arg = std::any_cast<Variable>(inst.args[0]);
                move_var_to_reg(arg, "%rax");
                output.appendf("    movq %rbp, %rsp\n");
                output.appendf("    popq %rbp\n");
                output.appendf("    ret\n");
                returned = true;
            }break;
            case Op::STORE_VAR: {
                Variable var1 = std::any_cast<Variable>(inst.args[0]);
                Variable var2 = std::any_cast<Variable>(inst.args[1]);

                deref_var_to_reg(var1, "%rax");
                move_reg_to_var("%rax", var2);
            }break;
            case Op::STORE_RET: {
                Variable var = std::any_cast<Variable>(inst.args[0]);
                move_reg_to_var("%rax", var);
            }break;
            case Op::INIT_STRING: {
                Variable str = std::any_cast<Variable>(inst.args[0]);
                auto storage_offset = current_string_count++*MAX_STRING_SIZE;
                output.appendf("    leaq {}+string_storage(%rip), {}\n", storage_offset, "%rax");
                move_reg_to_var("%rax", str);
            }break;
            case Op::CALL: {
                std::string func_name = std::any_cast<std::string>(inst.args[0]);
                VariableStorage args  = std::any_cast<VariableStorage>(inst.args[1]);

                call_func(func_name, args);

            }break;
            case Op::ADD: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                move_var_to_reg(lhs, "%rax");
                move_var_to_reg(rhs, "%rbx");
                output.appendf("    addq %rbx, %rax\n");
                move_reg_to_var("%rax", result);
            } break;
            case Op::SUB: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                move_var_to_reg(lhs, "%rax");
                move_var_to_reg(rhs, "%rbx");
                output.appendf("    subq %rbx, %rax\n");
                move_reg_to_var("%rax", result);
            } break;
            case Op::MUL: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                move_var_to_reg(lhs, "%rax");
                move_var_to_reg(rhs, "%rbx");
                output.appendf("    imulq %rbx, %rax\n"); // signed multiply
                move_reg_to_var("%rax", result);
            } break;
            case Op::DIV: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                move_var_to_reg(lhs, "%rax");
                output.appendf("    cqto\n");
                move_var_to_reg(rhs, "%rbx");
                output.appendf("    idivq %rbx\n");
                move_reg_to_var("%rax", result);
            } break;
            case Op::MOD: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                move_var_to_reg(lhs, "%rax");
                output.appendf("    cqto\n");
                move_var_to_reg(rhs, "%rbx");
                output.appendf("    idivq %rbx\n");
                output.appendf("    movq %rdx, %rax\n");
                move_reg_to_var("%rax", result);
            } break;
            case Op::LT: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                move_var_to_reg(lhs, "%rax");
                move_var_to_reg(rhs, "%rbx");
                output.appendf("    cmpq %rbx, %rax\n");
                output.appendf("    setl %al\n");
                output.appendf("    movzbq %al, %rax\n");
                move_reg_to_var("%rax", result);
            } break;
            case Op::EQ: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                move_var_to_reg(lhs, "%rax");
                move_var_to_reg(rhs, "%rbx");
                output.appendf("    cmpq %rbx, %rax\n");
                output.appendf("    sete %al\n");
                output.appendf("    movzbq %al, %rax\n");
                move_reg_to_var("%rax", result);
            } break;
            case Op::NE: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                move_var_to_reg(lhs, "%rax");
                move_var_to_reg(rhs, "%rbx");
                output.appendf("    cmpq %rbx, %rax\n");
                output.appendf("    setne %al\n");
                output.appendf("    movzbq %al, %rax\n");
                move_reg_to_var("%rax", result);
            } break;
            case Op::LE: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                move_var_to_reg(lhs, "%rax");
                move_var_to_reg(rhs, "%rbx");
                output.appendf("    cmpq %rbx, %rax\n");
                output.appendf("    setle %al\n");
                output.appendf("    movzbq %al, %rax\n");
                move_reg_to_var("%rax", result);
            } break;
            case Op::GT: { 
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                move_var_to_reg(lhs, "%rax");
                move_var_to_reg(rhs, "%rbx");
                output.appendf("    cmpq %rbx, %rax\n");
                output.appendf("    setg %al\n");
                output.appendf("    movzbq %al, %rax\n");
                move_reg_to_var("%rax", result);
            } break;
            case Op::GE: { 
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                move_var_to_reg(lhs, "%rax");
                move_var_to_reg(rhs, "%rbx");
                output.appendf("    cmpq %rbx, %rax\n");
                output.appendf("    setge %al\n");
                output.appendf("    movzbq %al, %rax\n");
                move_reg_to_var("%rax", result);
            } break;
            case Op::LAND: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                move_var_to_reg(lhs, "%rax");
                move_var_to_reg(rhs, "%rbx");
                output.appendf("    andq %rbx, %rax\n");
                output.appendf("    cmpq $0, %rax\n");
                output.appendf("    setne %al\n");
                output.appendf("    movzbq %al, %rax\n");
                move_reg_to_var("%rax", result);
            } break;
            case Op::LOR: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                move_var_to_reg(lhs, "%rax");
                move_var_to_reg(rhs, "%rbx");
                output.appendf("    orq %rbx, %rax\n");
                output.appendf("    cmpq $0, %rax\n");
                output.appendf("    setne %al\n");
                output.appendf("    movzbq %al, %rax\n");
                move_reg_to_var("%rax", result);
            } break;
        }
    }
    if (!returned) {
        move_var_to_reg({Type::Int_lit, "Int_lit", (int64_t)0}, "%rax");
        output.appendf("    movq %rbp, %rsp\n");
        output.appendf("    popq %rbp\n");
        output.appendf("    ret\n");
    }
}

void gnu_asm::call_func(std::string func_name, VariableStorage args) {
    if (args.size() > std::size(arg_register)) TODO("ERROR: stack arguments not implemented");

    for (size_t i = 0; i < args.size() && i < std::size(arg_register); i++) {
        if (args[i].type == Type::Int32_t)
            deref_var_to_reg(args[i], arg_register[i].first);
        else 
            deref_var_to_reg(args[i], arg_register[i].first);

    }

    output.appendf("    call {}\n", func_name);
}
void gnu_asm::deref_var_to_reg(Variable arg, std::string_view reg) {
    if (arg.deref_count == -1) {
        output.appendf("    leaq -{}(%rbp), {}\n", arg.offset, reg);
        return;
    }
    move_var_to_reg(arg, reg);
    while (arg.deref_count > 0) {
        output.appendf("    movq ({}), {}\n", reg, reg);
        arg.deref_count--;
    }
}


void gnu_asm::move_reg_to_reg(std::string_view reg1, std::string_view reg2) {
    output.appendf("    movq {}, {}\n", reg1, reg2);
}
void gnu_asm::move_var_to_reg(Variable arg, std::string_view reg) {
    if (arg.type == Type::String_lit)
        output.appendf("    leaq {}(%rip), {}\n", arg.name, arg_register[0].first);
    else if (arg.type == Type::Int_lit)
        output.appendf("    movq ${}, {}\n", std::any_cast<int64_t>(arg.value), reg);
    else if (arg.type == Type::Void_t)
        output.appendf("    movq $0, {}\n", reg);
    //else if (arg.type == Type::String_t)
    //    output.appendf("    leaq string_storage+{}(%rip), {}\n", arg.offset*MAX_STRING_SIZE, reg);
    else 
        output.appendf("    movq -{}(%rbp), {}\n", arg.offset, reg);
}
void gnu_asm::move_reg_to_var(std::string_view reg, Variable arg) {
    if (arg.type == Type::String_lit)
        TODO("can't move reg to string lit");
    else if (arg.type == Type::Int_lit)
        TODO("can't move reg to int lit");
    else if (arg.type == Type::Void_t) {
        std::println("reg {}, var {}", reg, arg.name);
        TODO("can't move reg to void");
    //} else if (arg.type == Type::String_t) {
    //    output.appendf("    leaq string_storage+{}(%rip), {}\n", arg.offset*MAX_STRING_SIZE, arg_register[0].first);
    //    move_reg_to_reg(reg, arg_register[1].first);
    //    output.appendf("    call strcpy\n");
    }else 
        output.appendf("    movq {}, -{}(%rbp)\n", reg, arg.offset);
}
void gnu_asm::move_var_to_var(Variable arg1, Variable arg2) {
    move_var_to_reg(arg1, "%rax");
    move_reg_to_var("%rax", arg2);
}

