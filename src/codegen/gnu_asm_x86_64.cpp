#include "codegen/gnu_asm_x86_64.h"
#include "context.h"
#include "tools/logger.h"
#include "types.h"
#include "tools/format.h"
#include <any>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <print>
#include <string>
#include <string_view>

int op = 0;

void gnu_asm::compileProgram() {
    if (m_program == nullptr) return;
    output.append(".section .text\n");
    for (const auto& func : m_program->func_storage) {
        if(func.external) {
            output.appendf(".global {}\n", func.name);
            output.appendf(".extern {}\n", func.link_name);
            if (func.name != func.link_name)
                output.appendf(".set    {}, {}\n", func.name, func.link_name);
            if (!ctx.libs.contains(func.lib))
                ctx.libs.insert(func.lib);
            if (!ctx.search_paths.contains(func.search_path))
                ctx.search_paths.insert(func.search_path);
        }else {
            compileFunction(func);
        }
    }

	output.appendf(".section .rodata\n");
    for (const auto& var : m_program->var_storage) {
        if (var.type == Type::String_lit)
            output.appendf("{}: .string \"{}\" \n", var.name, std::any_cast<std::string>(var.value));
    }


    std::ofstream outfile(f("{}/{}.s", build_path, input_no_extention));
    outfile << output;
    outfile.close();
}
void gnu_asm::compileFunction(Func func) {
    // if the function doesn't return you make it return 0
    bool returned = false;
	#ifdef WIN32
    std::pair<std::string_view, std::string_view> arg_register[] = {{"%rcx", "ecx"}, {"%rdx", "edx"}, {"%r8", "r8d"}, {"%r9", "r9d"}};
	#else	
    std::pair<std::string_view, std::string_view> arg_register[] = {
        {"%rdi", "%edi"}, {"%rsi", "%esi"}, {"%rdx", "%edx"}, {"%rcx", "%ecx"}, {"%r8", "%r8d"}, {"%r9", "%r9d"}
    };
	#endif

    output.appendf(".global {}\n", func.name);
    output.appendf("{}:\n", func.name);
    output.appendf("    pushq %rbp\n");
    output.appendf("    movq %rsp, %rbp\n");
    output.appendf("    andq $-16, %rsp\n");
    func.stack_size += func.stack_size % 16;
	if (func.stack_size < 32) func.stack_size = 32;
    output.appendf("    subq ${}, %rsp\n", func.stack_size);

    for (int i = 0; i < func.arguments_count; i++) {
        if (i < std::size(arg_register)) {
            if (func.arguments[i].type == Type::Int32_t)
                move_reg_to_var(arg_register[i].second, func.arguments[i]);       
            else 
                move_reg_to_var(arg_register[i].first, func.arguments[i]);       
        }
    }

    for (auto& inst : func.body) {
        output.appendf(".op_{}:\n", op++);
        switch (inst.op) {
            case Op::RETURN: {
                // NOTE: on Unix it takes the % of the return and 255 so the largest you can have is 255 and then it returns to 0
                Variable arg = std::any_cast<Variable>(inst.args[0]);
                move_var_to_reg(arg, "%rax");
                output.appendf("    movq %rbp, %rsp\n");
                output.appendf("    popq %rbp\n");
                output.appendf("    ret\n");
                returned = true;
            }break;
            case Op::LOAD_CONST: {
                output.appendf("    load({})\n", std::any_cast<int32_t>(inst.args[0]));
            }break;
            case Op::STORE_VAR: {
                Variable var1 = std::any_cast<Variable>(inst.args[0]);
                Variable var2 = std::any_cast<Variable>(inst.args[1]);
                move_var_to_var(var1, var2);
            }break;
            case Op::STORE_RET: {
                Variable var = std::any_cast<Variable>(inst.args[0]);
                move_reg_to_var("%rax", var);
            }break;
            case Op::CALL: {
                std::string func_name = std::any_cast<std::string>(inst.args[0]);
                VariableStorage args  = std::any_cast<VariableStorage>(inst.args[1]);

                if (args.size() > std::size(arg_register)) TODO("ERROR: stack arguments not implemented");

                for (size_t i = 0; i < args.size() && i < std::size(arg_register); i++) {
                    if (args[i].type == Type::Int32_t)
                        move_var_to_reg(args[i], arg_register[i].second);
                    else 
                        move_var_to_reg(args[i], arg_register[i].first);

                }

                output.appendf("    call {}\n", func_name);
            }break;
        }
    }
    if (!returned) {
        move_var_to_reg({Type::Int_lit, "Int_lit", 0}, "%rax");
        output.appendf("    movq %rbp, %rsp\n");
        output.appendf("    popq %rbp\n");
        output.appendf("    ret\n");
    }
}


void gnu_asm::move_var_to_reg(Variable arg, std::string_view reg) {
    if (arg.type == Type::String_lit)
        output.appendf("    leaq {}(%rip), {}\n", arg.name, reg);
    else if (arg.type == Type::Int_lit)
        output.appendf("    movq ${}, {}\n", std::any_cast<int>(arg.value), reg);
    else if (arg.type == Type::Void_t)
        output.appendf("    movq $0, {}\n", reg);
    else 
        output.appendf("    movq -{}(%rbp), {}\n", arg.offset, reg);
}
void gnu_asm::move_reg_to_var(std::string_view reg, Variable arg) {
    if (arg.type == Type::String_lit)
        output.appendf("    movq {}, ${}\n", reg, arg.name);
    else if (arg.type == Type::Int_lit)
        output.appendf("    movq {}, ${}\n", reg, std::any_cast<int>(arg.value));
    else if (arg.type == Type::Void_t)
        TODO("error trying to move register to void");
    else 
        output.appendf("    movq {}, -{}(%rbp)\n", reg, arg.offset);
}
void gnu_asm::move_var_to_var(Variable arg1, Variable arg2) {
    if (arg2.type == Type::String_lit)
        TODO("can't move var to lit");
        //move_var_to_reg(arg1, f("${}",arg2.name));
    else if (arg2.type == Type::Int_lit)
        TODO("can't move var to lit");
        //move_var_to_reg(arg1, f("${}", std::any_cast<int>(arg2.value)));
    else if (arg2.type == Type::Void_t)
        TODO("can't move var to void");
        //move_var_to_reg(arg1, "$0");
    else {
        if (arg1.type == Type::String_lit)
            move_reg_to_var(f("${}", arg1.name), arg2);
        else if (arg1.type == Type::Int_lit)
            move_reg_to_var(f("${}", std::any_cast<int>(arg1.value)), arg2);
        else if (arg1.type == Type::Void_t)
            move_reg_to_var("$0", arg2);
        else {
            move_var_to_reg(arg1, "%rax");
            move_reg_to_var("%rax", arg2);
        }
    }
}

