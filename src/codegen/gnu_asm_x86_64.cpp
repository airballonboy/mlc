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
#include <unordered_set>
#include <vector>

int op = 0;
#define MAX_STRING_SIZE 2048
size_t current_string_count = 0;
Func last_func{};

static std::vector<std::pair<Register, bool>> available_reg = {
    {Rax, true},
    {Rbx, true},
    {R13, true},
    {R14, true},
    {R15, true},
};
Register get_available_reg() {
    if (available_reg.size() < 1) TODO("no available Registers");
    Register reg;
    for (auto& [reg_, avail] : available_reg) {
        if (avail) {
            reg = reg_;
            avail = false;
            break;
        }
    }
    return reg;
}

void free_reg(Register reg) {
    for (auto& [reg_, avail] : available_reg) {
        if (reg_._64 == reg._64)
            if (avail == true)
                return;
    }
    if (REGS.contains(reg._64)) {
        for (auto& [reg_, avail] : available_reg) {
            if (reg_._64 == reg._64) {
                avail = true;
                break;
            }
        }
    } else 
        TODO("register doesn't exist");
}
bool is_int_type(Type t) {
    if (t == Type::Int64_t ||
        t == Type::Int32_t ||
        t == Type::Int16_t ||
        t == Type::Int8_t  ||
        t == Type::Char_t  ||
        t == Type::Size_t  ||
        t == Type::Bool_t
    )
        return true;
    return false;
}
Func get_func_from_module(Module mod, std::string name) {
    for (const auto& func : mod.func_storage) {
        if (name == func.name) return func;
    }
    for (auto [prefix, module] : mod.module_storage) {
        if (name.starts_with(prefix))
            return get_func_from_module(mod, name.erase(0, prefix.size()));
    }
    return {};
}
Func get_func_from_program(Program prog, std::string name) {
    for (auto func : prog.func_storage) 
        if (func.name == name) 
            return func;
    for (auto [prefix, mod] : prog.module_storage) {
        if (name.starts_with(prefix))
            return get_func_from_module(mod, name.erase(0, prefix.size()));
    }
    mlog::error(f("function {} was not found", name).c_str());
    exit(1);
}

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
        if (var.kind.literal && var.type_info->type == Type::String_t)
            output.appendf("{}: .string \"{}\" \n", var.name, std::any_cast<std::string>(var.value));
        if (var.kind.constant && var.kind.global) {
            output.appendf("{}:\n", var.name);
            if (is_int_type(var.type_info->type)) {
                if (var.type_info->size == 1)
                    output.appendf("    .byte {}\n", std::any_cast<int64_t>(var.value));
                else if (var.type_info->size == 2)
                    output.appendf("    .word {}\n", std::any_cast<int64_t>(var.value));
                else if (var.type_info->size == 4)
                    output.appendf("    .long {}\n", std::any_cast<int64_t>(var.value));
                else if (var.type_info->size == 8)
                    output.appendf("    .quad {}\n", std::any_cast<int64_t>(var.value));
            }
        }
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
    bool returned = false;

    output.appendf(".global {}\n", func.name);
    output.appendf("{}:\n", func.name);
    function_prologue();
    func.stack_size = ((func.stack_size + 15) & ~15) + 32;
    output.appendf("    subq ${}, %rsp\n", func.stack_size);

    for (int i = 0; i < func.arguments_count; i++) {
        if (i < std::size(arg_register)) {
            mov(arg_register[i], func.arguments[i]);       
        }
    }

    for (auto& inst : func.body) {
        returned = false;
        // Debug info maybe later?
        //output.appendf(".op_{}:\n", op++);
        switch (inst.op) {
            case Op::RETURN: {
                // NOTE: on Unix it takes the mod of the return and 256 so the largest you can have is 255 and after it returns to 0
                Variable arg = std::any_cast<Variable>(inst.args[0]);
                auto t = last_func.return_type;
                if (func.return_type.size <= 8) {
                    mov(arg, Rax);
                } else if (func.return_type.size <= 16) {
                    arg.size = 8;
                    mov(arg, Rax);
                    arg.offset -= 8;
                    mov(arg, Rcx);
                    mov(Rcx, Rdx);
                } else {
                    func.arguments[0].deref_count = 1;
                    mov(arg, func.arguments[0]);
                    mov(func.arguments[0], Rax);
                }
                function_epilogue();
                output.appendf("    ret\n");
                returned = true;
            }break;
            case Op::STORE_VAR: {
                Variable var1 = std::any_cast<Variable>(inst.args[0]);
                Variable var2 = std::any_cast<Variable>(inst.args[1]);
                if (var1.type_info->type != Type::String_t) {
                    mov(var1, var2);
                } else {
                    mov(var2, arg_register[0]);
                    mov(var1, arg_register[1]);
                    output.appendf("    call strcpy\n");
                }
            }break;
            case Op::STORE_RET: {
                Variable var = std::any_cast<Variable>(inst.args[0]);
                if (last_func.return_type.size <= 8) {
                    mov(Rax, var);
                } else if (last_func.return_type.size <= 16) {
                    var.size = 8;
                    mov(Rax, var);
                    var.offset -= 8;
                    var.size = var.type_info->size - 8;
                    mov(Rdx, var);
                } else {
                    TODO("unsupported");
                    //deref(Rax, 1);
                    //mov(Rax, var);
                }
                last_func = {};
            }break;
            case Op::INIT_STRING: {
                Variable str = std::any_cast<Variable>(inst.args[0]);
                auto storage_offset = current_string_count++*MAX_STRING_SIZE;
                auto reg = get_available_reg();
                output.appendf("    leaq {}+string_storage(%rip), {}\n", storage_offset, reg._64);
                mov(reg, str);
                free_reg(reg);
            }break;
            case Op::CALL: {
                std::string func_name = std::any_cast<std::string>(inst.args[0]);
                VariableStorage args  = std::any_cast<VariableStorage>(inst.args[1]);

                call_func(func_name, args);
                last_func = get_func_from_program(*m_program, func_name);
            }break;
            case Op::JUMP_IF_NOT: {
                std::string label = std::any_cast<std::string>(inst.args[0]);
                Variable    expr = std::any_cast<Variable>(inst.args[1]);
                auto reg = get_available_reg();
                mov(expr, reg);
                output.appendf("    testb {}, {}\n", reg._8, reg._8);
                output.appendf("    jz L{}\n", label);
                free_reg(reg);
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


                auto reg1 = get_available_reg();
                auto reg2 = get_available_reg();
                mov(lhs, reg1);
                mov(rhs, reg2);

                if (lhs.size != rhs.size) {
                    lhs.size = result.size;
                    rhs.size = result.size;
                }

                output.appendf("    {} {}, {}\n", INST_SIZE("add", result.size), REG_SIZE(reg2, rhs.size), REG_SIZE(reg1, lhs.size));
                mov(reg1, result);
                free_reg(reg1);
                free_reg(reg2);
            } break;
            case Op::SUB: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                auto reg1 = get_available_reg();
                auto reg2 = get_available_reg();
                mov(lhs, reg1);
                mov(rhs, reg2);
                output.appendf("    {} {}, {}\n", INST_SIZE("sub", result.size), REG_SIZE(reg2, result.size), REG_SIZE(reg1, result.size));
                mov(reg1, result);
                free_reg(reg1);
                free_reg(reg2);
            } break;
            case Op::MUL: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                auto reg1 = get_available_reg();
                auto reg2 = get_available_reg();
                mov(lhs, reg1);
                mov(rhs, reg2);
                output.appendf("    {} {}, {}\n", INST_SIZE("imul", lhs.size), REG_SIZE(reg2, lhs.size), REG_SIZE(reg1, lhs.size)); // signed multiply
                mov(reg1, result);
                free_reg(reg1);
                free_reg(reg2);
            } break;
            case Op::DIV: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                auto reg1 = get_available_reg();
                auto reg2 = get_available_reg();
                mov(lhs, reg1);
                output.appendf("    cqto\n");
                mov(rhs, reg2);
                output.appendf("    {} {}, {}\n", INST_SIZE("idiv", lhs.size), REG_SIZE(reg2, lhs.size), REG_SIZE(reg1, lhs.size));
                mov(reg1, result);
                free_reg(reg1);
                free_reg(reg2);
            } break;
            case Op::MOD: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                auto reg1 = get_available_reg();
                auto reg2 = get_available_reg();
                mov(lhs, reg1);
                output.appendf("    cqto\n");
                mov(rhs, reg2);
                output.appendf("    idivq {}\n", reg2._64);
                output.appendf("    movq {}, {}\n", Rdx._64, reg1._64);
                mov(reg1, result);
                free_reg(reg1);
                free_reg(reg2);
            } break;
            case Op::EQ: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                auto reg1 = get_available_reg();
                auto reg2 = get_available_reg();
                mov(lhs, reg1);
                mov(rhs, reg2);
                output.appendf("    {} {}, {}\n", INST_SIZE("cmp", lhs.size), REG_SIZE(reg1, lhs.size), REG_SIZE(reg2, lhs.size));
                output.appendf("    sete {}\n", reg1._8);
                output.appendf("    movzbq {}, {}\n", reg1._8, reg1._64);
                mov(reg1, result);
                free_reg(reg1);
                free_reg(reg2);
            } break;
            case Op::NE: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                auto reg1 = get_available_reg();
                auto reg2 = get_available_reg();
                mov(lhs, reg1);
                mov(rhs, reg2);
                output.appendf("    {} {}, {}\n", INST_SIZE("cmp", lhs.size), REG_SIZE(reg1, lhs.size), REG_SIZE(reg2, lhs.size));
                output.appendf("    setne {}\n", reg1._8);
                output.appendf("    movzbq {}, {}\n", reg1._8, reg1._64);
                mov(reg1, result);
                free_reg(reg1);
                free_reg(reg2);
            } break;
            case Op::LT: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                auto reg1 = get_available_reg();
                auto reg2 = get_available_reg();
                mov(lhs, reg2);
                mov(rhs, reg1);
                output.appendf("    {} {}, {}\n", INST_SIZE("cmp", lhs.size), REG_SIZE(reg1, lhs.size), REG_SIZE(reg2, lhs.size));
                output.appendf("    setl {}\n", reg1._8);
                output.appendf("    movzbq {}, {}\n", reg1._8, reg1._64);
                mov(reg1, result);
                free_reg(reg1);
                free_reg(reg2);
            } break;
            case Op::LE: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                auto reg1 = get_available_reg();
                auto reg2 = get_available_reg();
                mov(lhs, reg2);
                mov(rhs, reg1);
                output.appendf("    {} {}, {}\n", INST_SIZE("cmp", lhs.size), REG_SIZE(reg1, lhs.size), REG_SIZE(reg2, lhs.size));
                output.appendf("    setle {}\n", reg1._8);
                output.appendf("    movzbq {}, {}\n", reg1._8, reg1._64);
                mov(reg1, result);
                free_reg(reg1);
                free_reg(reg2);
            } break;
            case Op::GT: { 
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                auto reg1 = get_available_reg();
                auto reg2 = get_available_reg();
                mov(lhs, reg2);
                mov(rhs, reg1);
                output.appendf("    {} {}, {}\n", INST_SIZE("cmp", lhs.size), REG_SIZE(reg1, lhs.size), REG_SIZE(reg2, lhs.size));
                output.appendf("    setg {}\n", reg1._8);
                output.appendf("    movzbq {}, {}\n", reg1._8, reg1._64);
                mov(reg1, result);
                free_reg(reg1);
                free_reg(reg2);
            } break;
            case Op::GE: { 
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                auto reg1 = get_available_reg();
                auto reg2 = get_available_reg();
                mov(lhs, reg1);
                mov(rhs, reg2);
                output.appendf("    {} {}, {}\n", INST_SIZE("cmp", lhs.size), REG_SIZE(reg1, lhs.size), REG_SIZE(reg2, lhs.size));
                output.appendf("    setge {}\n", reg1._8);
                output.appendf("    movzbq {}, {}\n", reg1._8, reg1._64);
                mov(reg1, result);
                free_reg(reg1);
                free_reg(reg2);
            } break;
            case Op::LAND: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                auto reg1 = get_available_reg();
                auto reg2 = get_available_reg();
                mov(lhs, reg1);
                mov(rhs, reg2);
                output.appendf("    andq {}, {}\n", reg2._64, reg1._64);
                output.appendf("    cmpq $0, {}\n", reg1._64);
                output.appendf("    setne {}\n", reg1._8);
                output.appendf("    movzbq {}, {}\n", reg1._8, reg1._64);
                mov(reg1, result);
                free_reg(reg1);
                free_reg(reg2);
            } break;
            case Op::LOR: {
                Variable lhs = std::any_cast<Variable>(inst.args[0]);
                Variable rhs = std::any_cast<Variable>(inst.args[1]);
                Variable result = std::any_cast<Variable>(inst.args[2]);

                auto reg1 = get_available_reg();
                auto reg2 = get_available_reg();
                mov(lhs, reg1);
                mov(rhs, reg2);
                output.appendf("    orq {}, {}\n", reg2._64, reg1._64);
                output.appendf("    cmpq $0, {}\n", reg1._64);
                output.appendf("    setne {}\n", reg1._8);
                output.appendf("    movzbq {}, {}\n", reg1._8, reg1._64);
                mov(reg1, result);
                free_reg(reg1);
                free_reg(reg2);
            } break;
        }
    }
    if (!returned) {
        mov({.type_info = &type_infos.at("int8"), .name = "Int_lit", .value = (int64_t)0, .size = 1, .kind = {.literal = true}}, Rax);
        function_epilogue();
        output.appendf("    ret\n");
    }
}

void gnu_asm::call_func(std::string func_name, VariableStorage args) {
    if (args.size() > std::size(arg_register)) TODO("ERROR: stack arguments not implemented");

    for (size_t i = 0, j = 0; i < args.size() && j < std::size(arg_register); i++, j++) {
        if (args[i].type_info->type == Type::Struct_t) {
            if (args[i].deref_count > 0) {
                if (args[i].kind.pointer_count == args[i].deref_count)
                    args[i].size = get_struct_from_name(args[i].type_info->name).size;
            }
            if (args[i].size <= 8) {
                mov(args[i], arg_register[j]);
            } else if (args[i].size <= 16) {
                size_t orig_size = args[i].size;
                args[i].size = 8;
                mov(args[i], arg_register[j++]);
                if (args[i].deref_count == 0) {
                    args[i].size = orig_size-8;
                    args[i].offset -= 8;
                    mov(args[i], arg_register[j]);
                } else {
                    args[i].deref_count -= 1;
                    mov(args[i], arg_register[j]);
                    mov(8, arg_register[j], arg_register[j], orig_size-8);

                }
            } else {
                args[i].deref_count = -1;
                mov(args[i], arg_register[j]);
            }
        } else {
            if (args[i].deref_count == 0)
                mov(args[i], arg_register[j]);
            else 
                mov(args[i], arg_register[j]);
        }
    }

    output.appendf("    call {}\n", func_name);
}

Struct& gnu_asm::get_struct_from_name(std::string& name) {
    for (auto& strct_ : m_program->struct_storage) {
        if (strct_.name == name) return strct_;
    }
    TODO("struct not found");
}
void gnu_asm::mov(int64_t offset, Register src, Register dest, size_t size) {
    if (offset == 0) {
        output.appendf("    {} ({}), {}\n",
                           INST_SIZE("mov", size),
                           REG_SIZE(src, 8),
                           REG_SIZE(dest, size)
        );
    } else { 
        output.appendf("    {} {}({}), {}\n",
                           INST_SIZE("mov", size),
                           offset, REG_SIZE(src, 8),
                           REG_SIZE(dest, size)
        );
    }
}
void gnu_asm::mov(Register src, int64_t offset, Register dest, size_t size) {
    if (offset == 0) {
        output.appendf("    {} {}, ({})\n",
                           INST_SIZE("mov", size),
                           REG_SIZE(src, size),
                           REG_SIZE(dest, 8)
        );
    } else {
        output.appendf("    {} {}, {}({})\n",
                           INST_SIZE("mov", size),
                           REG_SIZE(src, size),
                           offset, REG_SIZE(dest, 8)
        );
    }
}
void gnu_asm::mov(Register src, Register dest, size_t size) {
    output.appendf("    {} {}, {}\n",
                       INST_SIZE("mov", size),
                       REG_SIZE(src, size),
                       REG_SIZE(dest, size)
    );
}
void gnu_asm::mov(int64_t int_value, Register dest, size_t size) {
    output.appendf("    {} ${}, {}\n",
                       INST_SIZE("mov", size),
                       int_value,
                       REG_SIZE(dest, size)
    );
}
void gnu_asm::mov(int64_t  int_value, std::string label, Register dest, size_t size) {
    output.appendf("    {} ${}, {}({})\n",
                       INST_SIZE("mov", size),
                       int_value,
                       label,
                       REG_SIZE(dest, 8)
    );
}
void gnu_asm::mov(int64_t int_value, int64_t offset, Register dest, size_t size) {
    if (offset == 0) {
        output.appendf("    {} ${}, ({})\n",
                           INST_SIZE("mov", size),
                           int_value,
                           REG_SIZE(dest, 8)
        );
    } else {
        output.appendf("    {} ${}, {}({})\n",
                           INST_SIZE("mov", size),
                           int_value,
                           offset, REG_SIZE(dest, 8)
        );
    }
}
void gnu_asm::mov(std::string global_label, Register src, Register dest, size_t size) {
    assert(size <= 8);
    output.appendf("    {} {}({}), {}\n",
                       INST_SIZE("mov", size),
                       global_label, REG_SIZE(src, 8),
                       REG_SIZE(dest, size)
    );
}
void gnu_asm::mov(Register src, std::string global_label, Register dest, size_t size) {
    assert(size <= 8);
    output.appendf("    {} {}, {}({})\n",
                       INST_SIZE("mov", size),
                       REG_SIZE(src, size),
                       global_label, REG_SIZE(dest, 8)
    );
}
void gnu_asm::mov(std::string global_label, Register src, Register dest) {
    mov(global_label, src, dest, 8);
}
void gnu_asm::mov(Register src, std::string global_label, Register dest) {
    mov(src, global_label, dest, 8);
}
void gnu_asm::mov(int64_t int_value, Register dest) {
    mov(int_value, dest, 8);
}
void gnu_asm::mov(int64_t int_value, int64_t offset, Register dest) {
    mov(int_value, offset, dest, 8);
}
void gnu_asm::mov(int64_t int_value, std::string label, Register dest) {
    mov(int_value, label, dest, 8);
}
void gnu_asm::mov(int64_t offset, Register src, Register dest) {
    mov(offset, src, dest, 8);
}
void gnu_asm::mov(Register src, int64_t offset, Register dest) {
    mov(src, offset, dest, 8);
}
void gnu_asm::mov(Register src, Register dest) {
    mov(src, dest, 8);
}
void gnu_asm::mov_member(Register src, Variable dest) {
    Variable current = dest;
    Variable* parent = dest.parent;
    size_t off = 0;
    Register reg = get_available_reg();
    if (parent == nullptr)
        off = current.offset;
    while (parent != nullptr) {
        parent = current.parent;
        if (parent == nullptr) {
            off = current.offset - off;
            break;
        }
        off += current.offset;
        if (parent->kind.pointer_count == 0) {
        } else {
            //mov(*parent->parent, reg);
            mov_member(*parent, reg);
            parent->deref_count = parent->kind.pointer_count - 1;
            deref(reg, parent->deref_count);
            //mov(current.offset, reg, reg);
            mov(src, off, reg, dest.size);
            free_reg(reg);
            return;
        }
        current = *current.parent;
    }
    if (dest.deref_count > 0) {
        dest.deref_count -= 1;
        mov(-off, Rbp, reg);
        deref(reg, dest.deref_count);
        mov(src, 0, reg, dest.size);
    } else {
        mov(src, -off, Rbp, dest.size);
    }
    free_reg(reg);
}
void gnu_asm::mov_member(Variable src, Register dest) {
    Variable current = src;
    Variable* parent = src.parent;
    bool in_rax = false;
    size_t off = 0;
    if (parent == nullptr)
        off = current.offset;
    while (parent != nullptr) {
        parent = current.parent;
        if (parent == nullptr) {
            off = current.offset - off;
            break;
        }
        //std::println("curr {}, par {}", current.name, parent->name);
        off += current.offset;
        if (parent->kind.pointer_count == 0) {
        } else {
            //std::println("deref => curr {}, par {}", current.name, parent->name);
            auto reg = get_available_reg();
            mov_member(*parent, reg);
            parent->deref_count = parent->kind.pointer_count - 1;
            deref(reg, parent->deref_count);

            if (src.deref_count == -1) {
                lea(off, reg, dest);
            } else {
                mov(off, reg, dest, src.size);
                if (src.deref_count > 0) {
                    deref(dest, src.deref_count);
                }
            }
            free_reg(reg);
            return;
        }
        current = *current.parent;
    }
    if (src.deref_count == -1) {
        lea(-off, Rbp, dest);
    } else {
        mov(-off, Rbp, dest, src.size);
        if (src.deref_count > 0) {
            deref(dest, src.deref_count);
        }
    }
}
void gnu_asm::mov(Variable src, Register dest) {
    //std::string_view& reg_name = REG_SIZE(dest, src.size);

    if (src.kind.literal && src.type_info->type == Type::String_t)
        lea(src.name, Rip, dest);
        //TODO("add lea func");
        //output.appendf("    leaq {}(%rip), {}\n", src.name, reg_name);
    else if (src.kind.literal && is_int_type(src.type_info->type))
        mov(std::any_cast<int64_t>(src.value), dest);
    else if (src.kind.global) 
        mov(src.name, Rip, dest);
    else if (src.type_info->type == Type::Void_t)
        mov(0, dest);
    else if (src.parent != nullptr) {
        mov_member(src, dest);
    } else if (src.deref_count > 0) {
        mov(-src.offset, Rbp, dest, src.size);
        deref(dest, src.deref_count);
    } else if (src.deref_count == -1) {
        lea(-src.offset, Rbp, dest);
    } else 
        mov(-src.offset, Rbp, dest, src.size);
}
void gnu_asm::mov(Register src, Variable dest) {
    if (dest.kind.constant) {
        TODO("can't move into a constant");
    } else if (dest.kind.literal) {
        TODO("can't mov into literals");
    } else if (dest.type_info->type == Type::Void_t) {
        TODO("can't mov into Void");
    } else if (dest.deref_count > 0) {
        Register reg = get_available_reg();
        dest.deref_count -= 1;
        mov(dest, reg);
        mov(src, 0, reg, dest.size);
        free_reg(reg);
    } else if (dest.kind.global) {
        mov(src, dest.name, Rip);
    } else {
        if (dest.parent != nullptr) {
            mov_member(src, dest);
        } else {
            if (dest.deref_count > 0) {
                dest.deref_count -= 1;
                mov(-dest.offset, Rbp, Rax);
                deref(Rax, dest.deref_count);
                mov(src, 0, Rax, dest.size);
            } else {
                mov(src, -dest.offset, Rbp, dest.size);
            }
        }
    }

}
void gnu_asm::mov(Variable src, Variable dest) {
    if (dest.kind.literal)
        TODO("can't mov into literals");
    if (dest.kind.constant)
        TODO("can't move into a constant");

    int64_t src_real_ptr_count = (src.kind.pointer_count-src.deref_count);
    int64_t dest_real_ptr_count = (dest.kind.pointer_count-dest.deref_count);

    if (src.kind.literal && is_int_type(src.type_info->type) && dest.parent == nullptr) {
        if (dest.type_info->type == Type::Struct_t)
            TODO(f("can't mov int literal into var of type {}", dest.type_info->name));
        if (dest.kind.global)
            mov(std::any_cast<int64_t>(src.value), dest.name, Rip, dest.size);
        else 
            mov(std::any_cast<int64_t>(src.value), -dest.offset, Rbp, dest.size);
    } else if (src.type_info->type == Type::Struct_t || dest.type_info->type == Type::Struct_t) {
        if (src.type_info->name != dest.type_info->name) 
            TODO(f("error trying assigning different structers to each other, {} {}", src.type_info->name, dest.type_info->name));
        Struct strct{};
        bool found = false;
        for (const auto& strct_ : m_program->struct_storage) {
            if (strct_.name == src.type_info->name) {
                found = true;
                strct = strct_;
                break;
            }
        }
        if (!found) TODO(f("struct {} wasn't found", src.type_info->name));
        // TODO: bug found where if you have something like this.color = ... it will be offset(%Rbp) instead of
        //       moving this to to a register and taking offset from it

        if (dest.kind.pointer_count > 0) {
            auto reg1 = get_available_reg();
            auto reg2 = get_available_reg();
            
            if (src_real_ptr_count != dest_real_ptr_count)
                TODO(f("trying to move Variable {} into {}, but kind is not the same", src.name, dest.name));
            if (dest_real_ptr_count == 0) {
                dest.deref_count -= 1;
                mov(dest, Rsi);

                if (src.kind.pointer_count > 0) {
                    src.deref_count -= 1;
                    mov(src, Rdi);
                } else {
                    lea(-src.offset, Rbp, Rdi);
                }

                mov(strct.size, Rcx);
                output.appendf("    rep movsb\n");
            } else if (dest_real_ptr_count > 0) {
                // this should be like call_func
                mov(src, reg1);
                mov(reg1, dest);
            } else 
                TODO("trying to store into a non lvalue");
            free_reg(reg1);
            free_reg(reg2);
        } else {
            if (src.deref_count > 0)
                mov(src, Rsi);
            else 
                lea(-src.offset, Rbp, Rsi);
            if (dest.deref_count > 0)
                mov(dest, Rdi);
            else 
                lea(-dest.offset, Rbp, Rdi);
            mov(strct.size, Rcx);
            output.appendf("    rep movsb\n");
        }
    } else {
        auto reg = get_available_reg();
        mov(src, reg);
        mov(reg, dest);
        
        free_reg(reg);
    }
}
void gnu_asm::deref(Register reg, int64_t deref_count) {
    if (deref_count == -1) {
        lea(reg, reg);
        return;
    } 
    while (deref_count > 0) {
        mov(0, reg, reg);
        deref_count -= 1;
    }
}
void gnu_asm::lea(Register src, Register dest) {
    output.appendf("    lea {}, {}\n", src._64, dest._64);
}
void gnu_asm::lea(std::string label, Register src, Register dest) {
    output.appendf("    lea {}({}), {}\n", label, src._64, dest._64);

}
void gnu_asm::lea(int64_t offset, Register src, Register dest) {
    output.appendf("    lea {}({}), {}\n", offset, src._64, dest._64);
}

void gnu_asm::function_prologue() {
    output.appendf("    pushq %rbp\n");
    output.appendf("    movq %rsp, %rbp\n");
    output.appendf("    pushq %rbx\n");
    output.appendf("    pushq %r13\n");
    output.appendf("    pushq %r14\n");
    output.appendf("    pushq %r15\n");
}
void gnu_asm::function_epilogue() {
    output.appendf("    popq %r15\n");
    output.appendf("    popq %r14\n");
    output.appendf("    popq %r13\n");
    output.appendf("    popq %rbx\n");
    output.appendf("    movq %rbp, %rsp\n");
    output.appendf("    popq %rbp\n");
}
