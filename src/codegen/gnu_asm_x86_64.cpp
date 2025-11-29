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


#define WARNING(...) std::println("\nWarning: {}\n", f(__VA_ARGS__))


#define REG_SIZE(REG, SIZE)   (SIZE) == 8 ? (REG)._64 : (SIZE) == 4 ? (REG)._32 : (SIZE) == 2 ? (REG)._16 : (REG)._8 
#define MOV_SIZE(SIZE)        (SIZE) == 8 ?   "movq"  : (SIZE) == 4 ?   "movl"  : (SIZE) == 2 ?   "movw"  :  "movb" 
#define INST_SIZE(INST, SIZE) (SIZE) == 8 ?  INST"q"  : (SIZE) == 4 ?   INST"l" : (SIZE) == 2 ?   INST"w" :  INST"b" 

#ifdef WIN32
static const Register arg_register[] = {
    Rcx, Rdx, R8, R9
};
#else	
static const Register arg_register[] = {
    Rdi, Rsi, Rdx, Rcx, R8, R9
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
    outfile.flush();
    outfile.close();
}
void gnu_asm::compileFunction(Func func) {
    // if the function doesn't return you make it return 0
    //asm("int3");
    bool returned = false;

    output.appendf(".global {}\n", func.name);
    output.appendf("{}:\n", func.name);
    output.appendf("    pushq %rbp\n");
    output.appendf("    movq %rsp, %rbp\n");
    output.appendf("    pushq %rbx\n");
    output.appendf("    pushq %r15\n");
    func.stack_size = ((func.stack_size + 15) & ~15) + 32;
    output.appendf("    subq ${}, %rsp\n", func.stack_size);

    for (int i = 0; i < func.arguments_count; i++) {
        if (i < std::size(arg_register)) {
            move_reg_to_var(arg_register[i], func.arguments[i]);       
        }
    }

    for (auto& inst : func.body) {
        returned = false;
        //output.appendf(".op_{}:\n", op++);
        switch (inst.op) {
            case Op::RETURN: {
                // NOTE: on Unix it takes the mod of the return and 256 so the largest you can have is 255 and after it returns to 0
                Variable arg = std::any_cast<Variable>(inst.args[0]);
                move_var_to_reg(arg, Rax);
                output.appendf("    popq %r15\n");
                output.appendf("    popq %rbx\n");
                output.appendf("    movq %rbp, %rsp\n");
                output.appendf("    popq %rbp\n");
                output.appendf("    ret\n");
                returned = true;
            }break;
            case Op::STORE_VAR: {
                Variable var1 = std::any_cast<Variable>(inst.args[0]);
                Variable var2 = std::any_cast<Variable>(inst.args[1]);

                //asm("int3");
                if (var1.type != Type::String_lit) {
                    //TODO: add this to parser as warning
                    //if (var2.size < var1.size){
                    //    WARNING("trying to assign a variable of size {} to a variable of size {} \n"
                    //            "  shrunk the bigger variable to fit into the smaller one",
                    //            var1.size, var2.size);
                    //    var1.size = var2.size;
                    //}
                    move_var_to_var(var1, var2);
                    //deref_var_to_reg(var1, Rax);
                    //move_reg_to_var(Rax, var2);
                } else {
                    move_var_to_reg(var2, arg_register[0]);
                    move_var_to_reg(var1, arg_register[1]);
                    output.appendf("    call strcpy\n");
                }
            }break;
            case Op::STORE_RET: {
                Variable var = std::any_cast<Variable>(inst.args[0]);
                move_reg_to_var(Rax, var);
            }break;
            case Op::DEREF_OFFSET: {
                size_t   offset = std::any_cast<size_t>  (inst.args[0]);
                Variable strct  = std::any_cast<Variable>(inst.args[1]);
                Variable var    = std::any_cast<Variable>(inst.args[2]);
                move_var_to_reg(strct, Rax);
                output.appendf("    {} {}({}), {}\n", INST_SIZE("mov", var.size), offset, Rax._64, REG_SIZE(Rax, var.size));
                move_reg_to_var(Rax, var);
            }break;
            case Op::INIT_STRING: {
                Variable str = std::any_cast<Variable>(inst.args[0]);
                auto storage_offset = current_string_count++*MAX_STRING_SIZE;
                output.appendf("    leaq {}+string_storage(%rip), {}\n", storage_offset, Rax._64);
                move_reg_to_var(Rax, str);
            }break;
            case Op::CALL: {
                std::string func_name = std::any_cast<std::string>(inst.args[0]);
                VariableStorage args  = std::any_cast<VariableStorage>(inst.args[1]);

                call_func(func_name, args);

            }break;
            case Op::JUMP_IF_NOT: {
                std::string label = std::any_cast<std::string>(inst.args[0]);
                Variable    expr = std::any_cast<Variable>(inst.args[1]);
                deref_var_to_reg(expr, Rax);
                output.appendf("    testb {}, {}\n", Rax._8, Rax._8);
                output.appendf("    jz L{}\n", label);
            }break;
            case Op::JUMP: {
                std::string label = std::any_cast<std::string>(inst.args[0]);
                output.appendf("    jmp L{}\n", label);
            }break;
            case Op::LABEL: {
                std::string label = std::any_cast<std::string>(inst.args[0]);

                output.appendf("L{}:\n", label);
            }break;
            case Op::ADD: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                //TODO: reseting registers
                //Variable zero = {.type = Type::Int_lit, .value = (int64_t)0, .size = 8};
                //move_var_to_reg(zero , Rax);
                //move_var_to_reg(zero , Rbx);

                deref_var_to_reg(lhs, Rax);
                deref_var_to_reg(rhs, Rbx);

                if (lhs.size != rhs.size) {
                    lhs.size = result.size;
                    rhs.size = result.size;
                }

                output.appendf("    {} {}, {}\n", INST_SIZE("add", result.size), REG_SIZE(Rbx, rhs.size), REG_SIZE(Rax, lhs.size));
                move_reg_to_var(Rax, result);
            } break;
            case Op::SUB: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                deref_var_to_reg(lhs, Rax);
                deref_var_to_reg(rhs, Rbx);
                output.appendf("    subq %rbx, %rax\n");
                move_reg_to_var(Rax, result);
            } break;
            case Op::MUL: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                deref_var_to_reg(lhs, Rax);
                deref_var_to_reg(rhs, Rbx);
                output.appendf("    imulq %rbx, %rax\n"); // signed multiply
                move_reg_to_var(Rax, result);
            } break;
            case Op::DIV: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                deref_var_to_reg(lhs, Rax);
                output.appendf("    cqto\n");
                deref_var_to_reg(rhs, Rbx);
                output.appendf("    idivq %rbx\n");
                move_reg_to_var(Rax, result);
            } break;
            case Op::MOD: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                deref_var_to_reg(lhs, Rax);
                output.appendf("    cqto\n");
                deref_var_to_reg(rhs, Rbx);
                output.appendf("    idivq %rbx\n");
                output.appendf("    movq %rdx, %rax\n");
                move_reg_to_var(Rax, result);
            } break;
            case Op::EQ: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                deref_var_to_reg(lhs, Rax);
                deref_var_to_reg(rhs, Rbx);
                output.appendf("    {} {}, {}\n", INST_SIZE("cmp", lhs.size), REG_SIZE(Rax, lhs.size), REG_SIZE(Rbx, lhs.size));
                output.appendf("    sete %al\n");
                output.appendf("    movzbq %al, %rax\n");
                move_reg_to_var(Rax, result);
            } break;
            case Op::NE: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                deref_var_to_reg(lhs, Rax);
                deref_var_to_reg(rhs, Rbx);
                output.appendf("    {} {}, {}\n", INST_SIZE("cmp", lhs.size), REG_SIZE(Rax, lhs.size), REG_SIZE(Rbx, lhs.size));
                output.appendf("    setne %al\n");
                output.appendf("    movzbq %al, %rax\n");
                move_reg_to_var(Rax, result);
            } break;
            case Op::LT: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                deref_var_to_reg(lhs, Rbx);
                deref_var_to_reg(rhs, Rax);
                output.appendf("    {} {}, {}\n", INST_SIZE("cmp", lhs.size), REG_SIZE(Rax, lhs.size), REG_SIZE(Rbx, lhs.size));
                output.appendf("    setl %al\n");
                output.appendf("    movzbq %al, %rax\n");
                move_reg_to_var(Rax, result);
            } break;
            case Op::LE: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                deref_var_to_reg(lhs, Rbx);
                deref_var_to_reg(rhs, Rax);
                output.appendf("    {} {}, {}\n", INST_SIZE("cmp", lhs.size), REG_SIZE(Rax, lhs.size), REG_SIZE(Rbx, lhs.size));
                output.appendf("    setle %al\n");
                output.appendf("    movzbq %al, %rax\n");
                move_reg_to_var(Rax, result);
            } break;
            case Op::GT: { 
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                deref_var_to_reg(lhs, Rbx);
                deref_var_to_reg(rhs, Rax);
                output.appendf("    {} {}, {}\n", INST_SIZE("cmp", lhs.size), REG_SIZE(Rax, lhs.size), REG_SIZE(Rbx, lhs.size));
                output.appendf("    setg %al\n");
                output.appendf("    movzbq %al, %rax\n");
                move_reg_to_var(Rax, result);
            } break;
            case Op::GE: { 
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                deref_var_to_reg(lhs, Rbx);
                deref_var_to_reg(rhs, Rax);
                output.appendf("    {} {}, {}\n", INST_SIZE("cmp", lhs.size), REG_SIZE(Rax, lhs.size), REG_SIZE(Rbx, lhs.size));
                output.appendf("    setge %al\n");
                output.appendf("    movzbq %al, %rax\n");
                move_reg_to_var(Rax, result);
            } break;
            case Op::LAND: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                deref_var_to_reg(lhs, Rax);
                deref_var_to_reg(rhs, Rbx);
                output.appendf("    andq %rbx, %rax\n");
                output.appendf("    cmpq $0, %rax\n");
                output.appendf("    setne %al\n");
                output.appendf("    movzbq %al, %rax\n");
                move_reg_to_var(Rax, result);
            } break;
            case Op::LOR: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                deref_var_to_reg(lhs, Rax);
                deref_var_to_reg(rhs, Rbx);
                output.appendf("    orq %rbx, %rax\n");
                output.appendf("    cmpq $0, %rax\n");
                output.appendf("    setne %al\n");
                output.appendf("    movzbq %al, %rax\n");
                move_reg_to_var(Rax, result);
            } break;
        }
    }
    if (!returned) {
        move_var_to_reg({.type = Type::Int_lit, .name = "Int_lit", .value = (int64_t)0, .size = 8}, Rax);
        output.appendf("    popq %r15\n");
        output.appendf("    popq %rbx\n");
        output.appendf("    movq %rbp, %rsp\n");
        output.appendf("    popq %rbp\n");
        output.appendf("    ret\n");
    }
}

void gnu_asm::call_func(std::string func_name, VariableStorage args) {
    if (args.size() > std::size(arg_register)) TODO("ERROR: stack arguments not implemented");

    for (size_t i = 0, j = 0; i < args.size() && j < std::size(arg_register); i++, j++) {
        if (args[i].type == Type::Struct_t) {
            if (args[i].size <= 8) {
                deref_var_to_reg(args[i], arg_register[j]);
            } else if (args[i].size <= 16) {
                size_t orig_size = args[i].size;
                args[i].size = 8;
                deref_var_to_reg(args[i], arg_register[j++]);
                args[i].size = orig_size-8;
                args[i].offset -= 8;
                deref_var_to_reg(args[i], arg_register[j]);
            } else {
                args[i].deref_count = -1;
                deref_var_to_reg(args[i], arg_register[j]);
            }
        } else {
            if (args[i].deref_count == 0)
                move_var_to_reg(args[i], arg_register[j]);
            else 
                deref_var_to_reg(args[i], arg_register[j]);
        }
    }

    output.appendf("    call {}\n", func_name);
}
void gnu_asm::deref_var_to_reg(Variable arg, Register reg) {
    if (arg.deref_count == -1) {
        output.appendf("    leaq -{}(%rbp), {}\n", arg.offset, reg._64);
        return;
    }
    arg.kind = {};
    move_var_to_reg(arg, reg);
    while (arg.deref_count > 0) {
        output.appendf("    movq ({}), {}\n", reg._64, reg._64);
        arg.deref_count--;
    }
}


void gnu_asm::move_reg_to_reg(Register reg1, Register reg2) {
    output.appendf("    movq {}, {}\n", reg1._64, reg2._64);
}
void gnu_asm::move_var_to_reg(Variable arg, Register reg) {
    std::string_view& reg_name = REG_SIZE(reg, arg.size);
    if (arg.type == Type::String_lit)
        output.appendf("    leaq {}(%rip), {}\n", arg.name, reg_name);
    else if (arg.type == Type::Int_lit)
        output.appendf("    {} ${}, {}\n", MOV_SIZE(arg.size), std::any_cast<int64_t>(arg.value), reg_name);
    else if (arg.type == Type::Void_t) {
        output.appendf("    movq $0, {}\n", reg._64);
    } else if (arg.kind.deref_offset != -1) {
        arg.deref_count = arg.kind.pointer_count - 1;
        size_t orig_size = arg.size;
        arg.size = 8;
        if (reg._64 != Rbx._64) {
            deref_var_to_reg(arg, Rbx);
            arg.size = orig_size;
            output.appendf("    {} {}({}), {}\n", INST_SIZE("mov", arg.size), arg.kind.deref_offset, Rbx._64, reg_name);
        }else { 
            deref_var_to_reg(arg, Rax);
            arg.size = orig_size;
            output.appendf("    {} {}({}), {}\n", INST_SIZE("mov", arg.size), arg.kind.deref_offset, Rax._64, reg_name);
        }
    } else 
        output.appendf("    {} -{}(%rbp), {}\n", MOV_SIZE(arg.size), arg.offset, reg_name);
}
void gnu_asm::move_reg_to_var(Register reg, Variable arg) {
    if (arg.type == Type::String_lit)
        TODO("can't move reg to string lit");
    else if (arg.type == Type::Int_lit)
        TODO("can't move reg to int lit");
    else if (arg.type == Type::Void_t) {
        std::println("reg {}, var {}", REG_SIZE(reg, arg.size), arg.name);
        TODO("can't move reg to void");
    } else if (arg.kind.deref_offset != -1) {
        arg.deref_count = arg.kind.pointer_count - 1;
        size_t orig_size = arg.size;
        arg.size = 8;
        if (reg._64 != Rbx._64) {
            deref_var_to_reg(arg, Rbx);
            arg.size = orig_size;
            output.appendf("    {} {}, {}({})\n", INST_SIZE("mov", arg.size), REG_SIZE(reg, arg.size), arg.kind.deref_offset, Rbx._64);
        }else { 
            deref_var_to_reg(arg, Rax);
            arg.size = orig_size;
            output.appendf("    {} {}, {}({})\n", INST_SIZE("mov", arg.size), REG_SIZE(reg, arg.size), arg.kind.deref_offset, Rax._64);
        }
    } else if (arg.deref_count > 0) {
        move_var_to_reg(arg, Rbx);
        while (arg.deref_count > 1) {
            output.appendf("    movq ({}), {}", Rbx._64, Rbx._64);
            arg.deref_count--;
        }
        output.appendf("    {} {}, ({})\n", MOV_SIZE(arg.size), REG_SIZE(reg, arg.size), Rbx._64);
    } else {
        output.appendf("    {} {}, -{}(%rbp)\n", MOV_SIZE(arg.size), REG_SIZE(reg, arg.size), arg.offset);
    }
}
void gnu_asm::move_var_to_var(Variable arg1, Variable arg2) {
    if (arg1.type == Type::Int_lit) {
        //asm("int3");
        int64_t int_val = std::any_cast<int64_t>(arg1.value);
        if (arg2.kind.deref_offset != -1) {
            arg2.deref_count = arg2.kind.pointer_count - 1;
            size_t orig_size = arg2.size;
            arg2.size = 8;
            deref_var_to_reg(arg2, Rbx);
            arg2.size = orig_size;
            output.appendf("    {} ${}, {}({})\n", INST_SIZE("mov", arg2.size), int_val, arg2.kind.deref_offset, Rbx._64);
        } else if (arg2.deref_count > 0) {
            move_var_to_reg(arg2, Rbx);
            while (arg2.deref_count > 1) {
                output.appendf("    movq ({}), {}", Rbx._64, Rbx._64);
                arg2.deref_count--;
            }
            output.appendf("    {} ${}, ({})\n", MOV_SIZE(arg2.size), int_val, REG_SIZE(Rbx, arg2.size));
        } else {
            output.appendf("    {} ${}, -{}(%rbp)\n",  MOV_SIZE(arg2.size), int_val, arg2.offset);
        }
    } else if (arg1.type == Type::Struct_t || arg2.type == Type::Struct_t) {
        if (arg1._type_name != arg2._type_name) TODO(f("error trying assigning different structers to each other, {} {}", arg1._type_name, arg2._type_name));
        Struct real_struct{};
        bool found = false;
        for (auto& strct : m_program->struct_storage) {
            if (strct.name == arg1._type_name) {
                real_struct = strct;
                found = true;
                break;
            }
        }
        if (!found) TODO("struct not found");
        if (arg1.deref_count > 0)
            output.appendf("    movq -{}(%rbp), %rsi\n", arg1.offset);
        else 
            output.appendf("    lea  -{}(%rbp), %rsi\n", arg1.offset);
        if (arg2.deref_count > 0)
            output.appendf("    movq -{}(%rbp), %rdi\n", arg2.offset);
        else 
            output.appendf("    lea  -{}(%rbp), %rdi\n", arg2.offset);
        output.appendf("    movq ${}, %rcx\n", real_struct.size);
        output.appendf("    rep movsb\n");
    } else if (arg2.kind.deref_offset != -1) {
        deref_var_to_reg(arg1, Rax);
        auto temp = arg2;
        temp.deref_count = arg2.kind.pointer_count - 1;
        size_t orig_size = arg2.size;
        arg2.size = 8;
        deref_var_to_reg(temp, Rbx);
        arg2.size = orig_size;
        output.appendf("    {} {}, {}({})\n", INST_SIZE("mov", arg2.size), REG_SIZE(Rax, arg2.size), arg2.kind.deref_offset, Rbx._64);
    } else {
        deref_var_to_reg(arg1, Rax);
        move_reg_to_var(Rax, arg2);
    }
}

