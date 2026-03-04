#include "codegen/gnu_asm_x86_64.h"
#include <any>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>
#include "codegen/asm_instruction.h"
#include "instruction.h"
#include "context.h"
#include "platform.h"
#include "tools/logger.h"
#include "tools/format.h"
#include "type_system/kind.h"
#include "type_system/struct.h"
#include "type_system/type_info.h"

int op = 0;
#define MAX_STRING_SIZE 2048
size_t current_string_count = 0;

static std::vector<std::pair<Register, bool>> available_float_reg = {
    {Xmm12, true},
    {Xmm13, true},
    {Xmm14, true},
    {Xmm15, true},
};
Register get_available_float_reg() {
    if (available_float_reg.size() < 1) TODO("no available Registers");
    Register reg;
    for (auto& [reg_, avail] : available_float_reg) {
        if (avail) {
            reg = reg_;
            avail = false;
            break;
        }
    }
    return reg;
}

void free_float_reg(Register reg) {
    for (auto& [reg_, avail] : available_float_reg) {
        if (reg_._64 == reg._64)
            if (avail == true)
                return;
    }
    if (xmm.contains(reg._64)) {
        for (auto& [reg_, avail] : available_float_reg) {
            if (reg_._64 == reg._64) {
                avail = true;
                break;
            }
        }
    } else 
        TODO("register doesn't exist");
}
static std::vector<std::pair<Register, bool>> available_reg = {
    {Rax, true},
    {Rbx, true},
    {R13, true},
    {R14, true},
    {R15, true},
};
Register get_available_int_reg() {
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

void free_int_reg(Register reg) {
    for (auto& [reg_, avail] : available_reg) {
        if (reg_._64 == reg._64)
            if (avail == true)
                return;
    }
    if (r64.contains(reg._64)) {
        for (auto& [reg_, avail] : available_reg) {
            if (reg_._64 == reg._64) {
                avail = true;
                break;
            }
        }
    } else 
        TODO("register doesn't exist");
}
bool is_float_reg(Register reg) {
    return xmm.contains(reg._64);
}

#define WARNING(...) mlog::println("\nWarning: {}\n", mlog::format(__VA_ARGS__))


#define REG_SIZE(REG, SIZE)   (SIZE) == 8 ? (REG)._64 : (SIZE) == 4 ? (REG)._32 : (SIZE) == 2 ? (REG)._16 : (REG)._8 
gnu_asm::gnu_asm(Program *prog) : BaseCodegenerator(prog) {
    if (m_program->platform == Platform::Windows) {
        arg_register = {
            Rcx, Rdx, R8, R9
        };
        arg_register_float = {
            Xmm0, Xmm1, Xmm2, Xmm3
        };
    } else if (m_program->platform == Platform::Linux) {
        arg_register = {
            Rdi, Rsi, Rdx, Rcx, R8, R9
        };

        arg_register_float = {
            Xmm0, Xmm1, Xmm2, Xmm3,
            Xmm4, Xmm5, Xmm6, Xmm7
        };
    }
}

void gnu_asm::compileProgram() {
    if (m_program == nullptr) return;
    output.append(".section .text\n");
    for (const auto& func : m_program->func_storage) {
        if (!func.external && (func.is_used || ctx.lib)) {
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


    output.append(".section .rodata\n");
    for (const auto& var : m_program->var_storage) {
        if (var.type.qualifiers & Qualifier::literal && var.type.info.id == TypeId::String) {
            output.appendf("{}: .string \"{}\"\n", var.name, var.String_val);
            continue;
        }
        if (var.type.qualifiers & Qualifier::literal && var.type.info.id == TypeId::Double) {
            output.appendf("{}: .double {}\n", var.name, var.Double_val);
            continue;
        }
        if (var.type.qualifiers & Qualifier::constant && var.type.qualifiers & Qualifier::global) {
            output.appendf("{}:\n", var.name);
            compileConstant(var);
        }
    }
    if (current_string_count != 0) {
        output.append(".align 8\n");
        output.append(".section .bss\n");
        output.appendf("string_storage: .skip {}\n", current_string_count*MAX_STRING_SIZE);
    }

    output.append(".section .text\n");
    output.append("// Externals\n");
    for (const auto& func : m_program->func_storage) {
        if (func.external && (func.is_used || ctx.lib)) {
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

    std::ofstream outfile(mlog::format("{}.s", (build_path/input_file.stem()).string()));
    outfile << output;
    outfile.flush();
    outfile.close();
}
void gnu_asm::compileConstant(Variable var) {
    if (var.type.info.kind == Kind::Int) {
        if (var.type.info.size == 1)
            output.appendf("    .byte 0x{:x}\n", var.Int_val);
        else if (var.type.info.size == 2)
            output.appendf("    .word 0x{:x}\n", var.Int_val);
        else if (var.type.info.size == 4)
            output.appendf("    .long 0x{:x}\n", var.Int_val);
        else if (var.type.info.size == 8)
            output.appendf("    .quad 0x{:x}\n", var.Int_val);
    }
    if (var.type.info.kind == Kind::Struct) {
        for (auto v2 : var.members) {
            compileConstant(v2);
        }
    }
}
void gnu_asm::compileFunction(Func func) {
    // if the function doesn't return you make it return 0
    bool returned = false;
    bool is_member = func.arguments.size() > 0 && func.name.starts_with(func.arguments[0].type.info.name) && func.arguments[0].name == "this";

    output.appendf(".global {}\n", func.name);
    output.appendf("{}:\n", func.name);
    function_prologue();
    func.stack_size = ((func.stack_size + 15) & ~15) + 32;
    sub.append(func.stack_size, Rsp);

    if (m_program->platform == Platform::Windows) {
        get_func_args_windows(func);
    } else {
        get_func_args_linux(func);
    }


    for (auto& inst : func.body) {
        returned = false;
        // Debug info maybe later?
        //output.appendf(".op_{}:\n", op++);
        switch (inst.op) {
            case Op::RETURN: {
                // NOTE: on Unix it takes the mod of the return and 256 so the largest you can have is 255 and after it returns to 0
                Variable arg = std::get<Variable>(inst.args[0]);
                if (m_program->platform == Platform::Windows) {
                    if (arg.type.info.kind == Kind::Float) {
                        mov_var(arg, Xmm0);
                    } else if (func.type.func_data->return_type->info.size <= 8 || func.type.info.kind == Kind::Pointer) {
                        mov_var(arg, Rax);
                    } else {
                        if (is_member) {
                            func.arguments[1].deref_count = 1;
                            mov_var(arg, func.arguments[1]);
                        } else {
                            func.arguments[0].deref_count = 1;
                            mov_var(arg, func.arguments[0]);
                        }
                    }
                } else {
                    if (func.type.func_data->return_type->info.size <= 8 || func.type.info.kind == Kind::Pointer) {
                        if (func.type.func_data->return_type->info.kind == Kind::Float) {
                            mov_var(arg, Xmm0);
                            cast_float_size(Xmm0, arg.size, func.type.func_data->return_type->info.size);
                        } else if (func.type.func_data->return_type->info.kind == Kind::Struct) {
                            if (Struct::get_from_name(arg.type.info.name, m_program->struct_storage).is_float_only)
                                mov_var(arg, Xmm0);
                            else 
                                mov_var(arg, Rax);
                        } else {
                            mov_var(arg, Rax);
                        }
                    } else if (func.type.func_data->return_type->info.size <= 16) {
                        if (Struct::get_from_name(arg.type.info.name, m_program->struct_storage).is_float_only) {
                            size_t size = arg.size;
                            arg.size = 8;
                            arg.type = type_infos.at("int64");
                            mov_var(arg, Xmm0);
                            arg.offset -= 8;
                            arg.size = size - 8;
                            mov_var(arg, Xmm1);
                        } else {
                            Register slots[2] = {Xmm0, Xmm1};
                            bool first_xmm = true;
                            size_t current_size = 0;
                            size_t i = 0;
                            for (auto elem : Struct::get_from_name(arg.type.info.name, m_program->struct_storage).var_storage) {
                                current_size += elem.size;
                                if (current_size <= 8) {
                                    if (elem.type.info.kind == Kind::Float) {
                                        slots[0] = (slots[0]._64 == Xmm0._64) ? Xmm0 : Rax;
                                    } else {
                                        slots[0] = Rax;
                                    }
                                    if (slots[0]._64 != Xmm0._64) {
                                        first_xmm = false;
                                        slots[1] = Xmm0;
                                    }
                                } else {
                                    if (elem.type.info.kind == Kind::Float) {
                                        if (first_xmm)
                                            slots[1] = (slots[1]._64 == Xmm1._64) ? Xmm1 : Rax;
                                        else
                                            slots[1] = (slots[1]._64 == Xmm0._64) ? Xmm0 : Rdx;
                                    } else {
                                        if (first_xmm)
                                            slots[1] = Rax;
                                        else 
                                            slots[1] = Rdx;
                                    }
                                }
                            }
                            arg.size = 8;
                            mov_var(arg, slots[0]);
                            arg.size = current_size - 8;
                            arg.offset -= 8;
                            mov_var(arg, slots[1]);
                        }
                    } else {
                        if (is_member) {
                            func.arguments[1].deref_count = 1;
                            mov_var(arg, func.arguments[1]);
                        } else {
                            func.arguments[0].deref_count = 1;
                            mov_var(arg, func.arguments[0]);
                        }
                    }
                }
                add.append(func.stack_size, Rsp);
                function_epilogue();
                output.append("    ret\n");
                returned = true;
            }break;
            case Op::STORE_VAR: {
                Variable var1 = std::get<Variable>(inst.args[0]);
                Variable var2 = std::get<Variable>(inst.args[1]);
                if (var1.type.info.kind == Kind::Void || var2.type.info.kind == Kind::Void)
                    break;
                if (var1.type.info.id != TypeId::String) {
                    mov_var(var1, var2);
                } else {
                    mov_var(var2, arg_register[0]);
                    mov_var(var1, arg_register[1]);
                    output.append("    call strcpy\n");
                }
            }break;
            case Op::INIT_STRING: {
                Variable str = std::get<Variable>(inst.args[0]);
                auto storage_offset = current_string_count++*MAX_STRING_SIZE;
                auto reg = get_available_int_reg();
                output.appendf("    leaq {}+string_storage(%rip), {}\n", storage_offset, reg._64);
                mov_var(reg, str);
                free_int_reg(reg);
            }break;
            case Op::CALL: {
                Func func             = std::get<Func>(inst.args[0]);
                VariableStorage args  = std::get<VariableStorage>(inst.args[1]);
                Variable ret_address  = std::get<Variable>(inst.args[2]);

                call_func(func, args);
                if (ret_address.type.info.kind != Kind::Void) {
                    if (m_program->platform == Platform::Windows) {
                        if (ret_address.type.info.kind == Kind::Float) {
                            mov_var(Xmm0, ret_address);
                        } else if (func.type.func_data->return_type->info.size <= 8 || func.type.info.kind == Kind::Pointer) {
                            mov_var(Rax, ret_address);
                        }
                    } else {
                        if (func.type.func_data->return_type->info.size <= 8 || func.type.info.kind == Kind::Pointer) {
                            if (ret_address.type.info.kind == Kind::Float) {
                                mov_var(Xmm0, ret_address);
                            } else if (ret_address.type.info.kind == Kind::Struct) {
                                if (Struct::get_from_name(ret_address.type.info.name, m_program->struct_storage).is_float_only)
                                    mov_var(Xmm0, ret_address);
                                else 
                                    mov_var(Rax, ret_address);
                            } else {
                                mov_var(Rax, ret_address);
                            }
                        } else if (func.type.func_data->return_type->info.size <= 16) {
                            if (Struct::get_from_name(ret_address.type.info.name, m_program->struct_storage).is_float_only) {
                                size_t size = ret_address.size;
                                ret_address.size = 8;
                                TypeInfo ti = ret_address.type.info;
                                ret_address.type = type_infos.at("int64");
                                mov_var(Xmm0, ret_address);
                                ret_address.offset -= 8;
                                ret_address.size = size - 8;
                                mov_var(Xmm1, ret_address);
                            } else {
                                Register slots[2] = {Xmm0, Xmm1};
                                bool first_xmm = true;
                                size_t current_size = 0;
                                size_t i = 0;
                                for (auto elem : Struct::get_from_name(ret_address.type.info.name, m_program->struct_storage).var_storage) {
                                    current_size += elem.size;
                                    if (current_size <= 8) {
                                        if (elem.type.info.kind == Kind::Float) {
                                            slots[0] = (slots[0]._64 == Xmm0._64) ? Xmm0 : Rax;
                                        } else {
                                            slots[0] = Rax;
                                        }
                                        if (slots[0]._64 != Xmm0._64) {
                                            first_xmm = false;
                                            slots[1] = Xmm0;
                                        }
                                    } else {
                                        if (elem.type.info.kind == Kind::Float) {
                                            if (first_xmm)
                                                slots[1] = (slots[1]._64 == Xmm1._64) ? Xmm1 : Rax;
                                            else
                                                slots[1] = (slots[1]._64 == Xmm0._64) ? Xmm0 : Rdx;
                                        } else {
                                            if (first_xmm)
                                                slots[1] = Rax;
                                            else 
                                                slots[1] = Rdx;
                                        }
                                    }
                                }
                                ret_address.size = 8;
                                mov_var(slots[0], ret_address);
                                ret_address.size = current_size - 8;
                                ret_address.offset -= 8;
                                mov_var(slots[1], ret_address);
                            }
                        }
                    }
                }
            }break;
            case Op::JUMP_IF_NOT: {
                std::string label = std::get<std::string>(inst.args[0]);
                Variable    expr = std::get<Variable>(inst.args[1]);
                auto reg = get_available_int_reg();
                mov_var(expr, reg);
                output.appendf("    testb {}, {}\n", reg._8, reg._8);
                output.appendf("    jz L{}\n", label);
                free_int_reg(reg);
            }break;
            case Op::JUMP: {
                std::string label = std::get<std::string>(inst.args[0]);
                output.appendf("    jmp L{}\n", label);
            }break;
            case Op::LABEL: {
                std::string label = std::get<std::string>(inst.args[0]);

                output.appendf("L{}:\n", label);
            }break;
            case Op::ADD: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);


                if (lhs.type.info.kind == Kind::Float || rhs.type.info.kind == Kind::Float) {
                    auto reg1 = get_available_float_reg();
                    auto reg2 = get_available_float_reg();
                    mov_var(lhs, reg1);
                    mov_var(rhs, reg2);

                    size_t size = std::max<size_t>(result.size, 4);
                    cast_float_size(reg1, lhs.size, size);
                    cast_float_size(reg2, rhs.size, size);

                    adds.append(reg2, reg1, size);
                    mov_var(reg1, result);
                    free_float_reg(reg1);
                    free_float_reg(reg2);
                } else {
                    auto reg1 = get_available_int_reg();
                    auto reg2 = get_available_int_reg();
                    mov_var(lhs, reg1);
                    mov_var(rhs, reg2);

                    if (lhs.size != rhs.size) {
                        lhs.size = result.size;
                        rhs.size = result.size;
                    }

                    size_t size = std::max<size_t>(result.size, 2);
                    add.append(reg2, reg1, size);
                    mov_var(reg1, result);
                    free_int_reg(reg1);
                    free_int_reg(reg2);
                }
            } break;
            case Op::SUB: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);

                if (lhs.type.info.kind == Kind::Float || rhs.type.info.kind == Kind::Float) {
                    auto reg1 = get_available_float_reg();
                    auto reg2 = get_available_float_reg();
                    mov_var(lhs, reg1);
                    mov_var(rhs, reg2);

                    size_t size = std::max<size_t>(result.size, 4);
                    cast_float_size(reg1, lhs.size, size);
                    cast_float_size(reg2, rhs.size, size);

                    subs.append(reg2, reg1, size);
                    mov_var(reg1, result);
                    free_float_reg(reg1);
                    free_float_reg(reg2);
                } else {
                    auto reg1 = get_available_int_reg();
                    auto reg2 = get_available_int_reg();
                    mov_var(lhs, reg1);
                    mov_var(rhs, reg2);
                    size_t size = std::max<size_t>(result.size, 2);
                    sub.append(reg2, reg1, size);
                    mov_var(reg1, result);
                    free_int_reg(reg1);
                    free_int_reg(reg2);
                }
            } break;
            case Op::MUL: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);

                if (lhs.type.info.kind == Kind::Float || rhs.type.info.kind == Kind::Float) {
                    auto reg1 = get_available_float_reg();
                    auto reg2 = get_available_float_reg();
                    mov_var(lhs, reg1);
                    mov_var(rhs, reg2);

                    size_t size = std::max<size_t>(result.size, 4);
                    cast_float_size(reg1, lhs.size, size);
                    cast_float_size(reg2, rhs.size, size);

                    muls.append(reg2, reg1, size);
                    mov_var(reg1, result);
                    free_float_reg(reg1);
                    free_float_reg(reg2);
                } else {
                    auto reg1 = get_available_int_reg();
                    auto reg2 = get_available_int_reg();
                    mov_var(lhs, reg1);
                    mov_var(rhs, reg2);
                    size_t size = std::max<size_t>(result.size, 2);
                    imul.append(reg2, reg1, size);
                    mov_var(reg1, result);
                    free_int_reg(reg1);
                    free_int_reg(reg2);
                }
            } break;
            case Op::DIV: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);

                auto reg1 = get_available_int_reg();
                auto reg2 = get_available_int_reg();
                mov_var(lhs, reg1);
                output.append("    cqto\n");
                mov_var(rhs, reg2);
                size_t size = std::max<size_t>(result.size, 2);
                idiv.append(reg2, reg1, size);
                mov_var(reg1, result);
                free_int_reg(reg1);
                free_int_reg(reg2);
            } break;
            case Op::MOD: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);

                auto reg1 = get_available_int_reg();
                auto reg2 = get_available_int_reg();
                mov_var(lhs, reg1);
                output.append("    cqto\n");
                mov_var(rhs, reg2);
                idiv.append(reg2, 8);
                mov.append(Rdx, reg1 ,8);
                mov_var(reg1, result);
                free_int_reg(reg1);
                free_int_reg(reg2);
            } break;
            case Op::EQ: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);

                auto reg1 = get_available_int_reg();
                auto reg2 = get_available_int_reg();
                mov_var(lhs, reg1);
                mov_var(rhs, reg2);
                cmp.append(reg1, reg2, lhs.size);
                output.appendf("    sete {}\n", reg1._8);
                output.appendf("    movzbq {}, {}\n", reg1._8, reg1._64);
                mov_var(reg1, result);
                free_int_reg(reg1);
                free_int_reg(reg2);
            } break;
            case Op::NE: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);

                auto reg1 = get_available_int_reg();
                auto reg2 = get_available_int_reg();
                mov_var(lhs, reg1);
                mov_var(rhs, reg2);
                cmp.append(reg1, reg2, lhs.size);
                output.appendf("    setne {}\n", reg1._8);
                output.appendf("    movzbq {}, {}\n", reg1._8, reg1._64);
                mov_var(reg1, result);
                free_int_reg(reg1);
                free_int_reg(reg2);
            } break;
            case Op::LT: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);

                auto reg1 = get_available_int_reg();
                auto reg2 = get_available_int_reg();
                mov_var(lhs, reg2);
                mov_var(rhs, reg1);
                cmp.append(reg1, reg2, lhs.size);
                output.appendf("    setl {}\n", reg1._8);
                output.appendf("    movzbq {}, {}\n", reg1._8, reg1._64);
                mov_var(reg1, result);
                free_int_reg(reg1);
                free_int_reg(reg2);
            } break;
            case Op::LE: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);

                auto reg1 = get_available_int_reg();
                auto reg2 = get_available_int_reg();
                mov_var(lhs, reg2);
                mov_var(rhs, reg1);
                cmp.append(reg1, reg2, lhs.size);
                output.appendf("    setle {}\n", reg1._8);
                output.appendf("    movzbq {}, {}\n", reg1._8, reg1._64);
                mov_var(reg1, result);
                free_int_reg(reg1);
                free_int_reg(reg2);
            } break;
            case Op::GT: { 
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);

                auto reg1 = get_available_int_reg();
                auto reg2 = get_available_int_reg();
                mov_var(lhs, reg2);
                mov_var(rhs, reg1);
                cmp.append(reg1, reg2, lhs.size);
                output.appendf("    setg {}\n", reg1._8);
                output.appendf("    movzbq {}, {}\n", reg1._8, reg1._64);
                mov_var(reg1, result);
                free_int_reg(reg1);
                free_int_reg(reg2);
            } break;
            case Op::GE: { 
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);

                auto reg1 = get_available_int_reg();
                auto reg2 = get_available_int_reg();
                mov_var(lhs, reg2);
                mov_var(rhs, reg1);
                cmp.append(reg1, reg2, lhs.size);
                output.appendf("    setge {}\n", reg1._8);
                output.appendf("    movzbq {}, {}\n", reg1._8, reg1._64);
                mov_var(reg1, result);
                free_int_reg(reg1);
                free_int_reg(reg2);
            } break;
            case Op::LAND: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);

                auto reg1 = get_available_int_reg();
                auto reg2 = get_available_int_reg();
                mov_var(lhs, reg1);
                mov_var(rhs, reg2);
                output.appendf("    andq {}, {}\n", reg2._64, reg1._64);
                cmp.append(0, reg1, 8);
                output.appendf("    setne {}\n", reg1._8);
                output.appendf("    movzbq {}, {}\n", reg1._8, reg1._64);
                mov_var(reg1, result);
                free_int_reg(reg1);
                free_int_reg(reg2);
            } break;
            case Op::LOR: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);

                auto reg1 = get_available_int_reg();
                auto reg2 = get_available_int_reg();
                mov_var(lhs, reg1);
                mov_var(rhs, reg2);
                output.appendf("    orq {}, {}\n", reg2._64, reg1._64);
                cmp.append(0, reg1, 8);
                output.appendf("    setne {}\n", reg1._8);
                output.appendf("    movzbq {}, {}\n", reg1._8, reg1._64);
                mov_var(reg1, result);
                free_int_reg(reg1);
                free_int_reg(reg2);
            } break;
        }
    }
    if (!returned) {
        mov.append(0, Rax, 8);
        function_epilogue();
        output.appendf("    ret\n");
    }
}
void gnu_asm::get_func_args_windows(Func func) {
    size_t temp_float_size = 0;
    Register reg1{};
    Register reg2{};
    size_t current_stack_offset = 32;

    for (int j = 0, i = 0, f = 0; j < func.arguments.size(); i++, j++, f++) {
        auto& arg = func.arguments[j];
        bool is_stack      = i   > arg_register.size()-1;
        bool is_next_stack = i+1 > arg_register.size()-1;
        if (is_stack) {
            reg1 = get_available_int_reg();
            reg2 = get_available_float_reg();
        } else if (is_next_stack) {
            reg1 = arg_register[i];
            reg2 = arg_register_float[f];
        } else {
            reg1 = arg_register[i];
            reg2 = arg_register_float[f];
        }
        if (is_stack) {
            if (arg.type.info.kind == Kind::Float && !func.c_variadic) {
                cast_float_size(reg2, arg.size, temp_float_size);
				mov.append(current_stack_offset, Rsp, reg2);
			} else {
				mov.append(current_stack_offset, Rsp, reg1);
			}
			current_stack_offset += std::max(8, (int)arg.size);
        }
        if (is_next_stack) {
            if (arg.size > 8) {
                mov.append(current_stack_offset+8, Rsp, reg2);
                current_stack_offset += 8;
            }
        }
        if (arg.type.info.kind == Kind::Struct) {
            auto strct = Struct::get_from_name(arg.type.info.name, m_program->struct_storage);
            if (arg.deref_count > 0) {
                if (get_ptr_count(arg.type) == arg.deref_count)
                    arg.size = strct.size;
            }
            if (arg.size <= 8) {
                mov_var(reg1, arg);
            } else {
                arg.deref_count = -1;
                mov_var(reg1, arg);
            }
        } else {
            if (arg.type.info.kind == Kind::Float) {
                mov_var(reg2, arg);
            } else {
                mov_var(arg, reg1);
            }
        }
        free_int_reg(reg1);
        free_float_reg(reg2);
    }
}
void gnu_asm::get_func_args_linux(Func func) {
    size_t f = 0;
    Register reg1;
    Register reg2;
    Register reg3;
    Register reg4;
    size_t current_stack_offset = 0;

    for (int j = 0, i = 0, f = 0; j < func.arguments.size(); i++, j++, f++) {
        auto& arg = func.arguments[j];
        bool is_stack            = i   > arg_register.size()-1;
        bool is_next_stack       = i+1 > arg_register.size()-1;
        bool is_stack_float      = f   > arg_register_float.size()-1;
        bool is_next_stack_float = f+1 > arg_register_float.size()-1;
        if (is_stack) {
            reg1 = get_available_int_reg();
            reg2 = get_available_int_reg();
        } else if (is_next_stack) {
            reg1 = arg_register[i];
            reg2 = get_available_int_reg();
        } else {
            reg1 = arg_register[i];
            reg2 = arg_register[i+1];
        }
        if (is_stack_float) {
            reg3 = get_available_float_reg();
            reg4 = get_available_float_reg();
        } else if (is_next_stack_float) {
            reg3 = arg_register_float[f];
            reg4 = get_available_float_reg();
        } else {
            reg3 = arg_register_float[f];
            reg4 = arg_register_float[f+1];
        }
        if (is_stack) {
            mov.append(current_stack_offset, Rsp, reg1);
            if (arg.size > 8) {
                mov.append(current_stack_offset+8, Rsp, reg2);
            }
            current_stack_offset += std::max(8, (int)arg.size);
        }
        if (is_stack_float) {
            mov.append(current_stack_offset, Rsp, reg3);
            current_stack_offset += std::max(8, (int)arg.size);
        }
        if (is_next_stack) {
            if (arg.size > 8) {
                mov.append(current_stack_offset+8, Rsp, reg2);
                current_stack_offset += 8;
            }
        }
        if (is_next_stack_float) {
            if (arg.size > 8) {
                mov.append(current_stack_offset+8, Rsp, reg4);
                current_stack_offset += 8;
            }
        }
        if (arg.type.info.kind == Kind::Struct) {
            auto strct = Struct::get_from_name(arg.type.info.name, m_program->struct_storage);
            if (arg.deref_count > 0) {
                if (get_ptr_count(arg.type) == arg.deref_count)
                    arg.size = strct.size;
            }
            if (arg.size <= 8) {
                if (strct.is_float_only && arg.type.info.kind != Kind::Pointer) {
                    mov_var(reg3, arg);
                    i--;
                } else {
                    mov_var(reg1, arg);
                    f--;
                }
            } else if (arg.size <= 16) {
                if (strct.is_float_only) {
                    size_t orig_size = arg.size;
                    arg.size = 8;
                    mov_var(reg3, arg);
                    arg.size = orig_size-8;
                    arg.offset -= 8;
                    mov_var(reg4, arg);
                    f++;
                    i--;
                } else {
                    size_t orig_size = arg.size;
                    arg.size = 8;
                    mov_var(reg1, arg);
                    arg.size = orig_size-8;
                    arg.offset -= 8;
                    mov_var(reg2, arg);
                    f--;
                }
            } else {
                output.appendf("    cld\n");
                mov_var(reg1, arg);
                mov.append(reg1, Rsi);
                lea.append(-arg.offset, Rbp, Rdi);
                mov.append(strct.size, Rcx);
                output.append("rep movsb\n");
            }
        } else {
            if (arg.type.info.kind == Kind::Float) {
                mov_var(reg3, arg);
                i--;
            } else {
                f--;
                mov_var(reg1, arg);
            }
        }
        free_int_reg(reg1);
        free_int_reg(reg2);
        free_float_reg(reg3);
        free_float_reg(reg4);
    }
}

void gnu_asm::call_func_windows(Func func, VariableStorage args) {
    size_t temp_float_size = 0;
    Register reg1{};
    Register reg2{};
    size_t current_stack_offset = 32;
    for (size_t i = 0, j = 0; i < args.size(); i++, j++) {
        if (args[i].type.info.kind == Kind::Float && func.c_variadic)
            temp_float_size = 8;
        else if (!func.c_variadic)
            temp_float_size = func.arguments[i].size;
        bool is_stack      = j   > arg_register.size()-1;
        bool is_next_stack = j+1 > arg_register.size()-1;
        if (is_stack) {
            reg1 = get_available_int_reg();
            reg2 = get_available_float_reg();
        } else if (is_next_stack) {
            reg1 = arg_register[j];
            reg2 = arg_register_float[j];
        } else {
            reg1 = arg_register[j];
            reg2 = arg_register_float[j];
        }
        if (args[i].type.info.kind == Kind::Struct) {
            auto strct = Struct::get_from_name(args[i].type.info.name, m_program->struct_storage);
            if (args[i].deref_count > 0) {
                if (get_ptr_count(args[i].type) == args[i].deref_count)
                    args[i].size = strct.size;
            }
            if (args[i].size <= 8) {
                mov_var(args[i], reg1);
            } else {
                args[i].deref_count = -1;
                mov_var(args[i], reg1);
            }
        } else {
            if (args[i].type.info.kind == Kind::Float) {
                if (func.c_variadic) {
                    if (args[i].size != temp_float_size) {
                        mov_var(args[i], reg2);
                        cast_float_size(reg2, args[i].size, temp_float_size);
                        mov.append(reg2, reg1);
                    } else {
                        mov_var(args[i], reg1);
                    }
                } else {
                    mov_var(args[i], reg2);
                    cast_float_size(reg2, args[i].size, temp_float_size);
                }
            } else {
                mov_var(args[i], reg1);
                if (func.c_variadic) {
                    if (args[i].size == 1)
                        output.appendf("    movsbl {}, {}\n", reg1._8, reg1._32);
                    if (args[i].size == 2)
                        output.appendf("    movswl {}, {}\n", reg1._16, reg1._32);
                }
            }
        }
        if (is_stack) {
            if (args[i].type.info.kind == Kind::Float && !func.c_variadic) {
                cast_float_size(reg2, args[i].size, temp_float_size);
				mov.append(reg2, current_stack_offset, Rsp);
			} else {
				mov.append(reg1, current_stack_offset, Rsp);
			}
			current_stack_offset += std::max(8, (int)args[i].size);
        }
        if (is_next_stack) {
            if (args[i].size > 8) {
                mov.append(reg2, current_stack_offset+8, Rsp);
                current_stack_offset += 8;
            }
        }
        free_int_reg(reg1);
        free_float_reg(reg2);
    }
    output.appendf("    call {}\n", func.name);
}
void gnu_asm::call_func_linux(Func func, VariableStorage args) {
    size_t f = 0;
    size_t temp_float_size = 0;
    Register reg1;
    Register reg2;
    Register reg3;
    Register reg4;
    size_t current_stack_offset = 0;

    for (size_t i = 0, j = 0; i < args.size(); i++, j++, f++) {
        if (args[i].type.info.kind == Kind::Float && func.c_variadic)
            temp_float_size = 8;
        else if (!func.c_variadic)
            temp_float_size = func.arguments[i].size;
        bool is_stack            = j   > arg_register.size()-1;
        bool is_next_stack       = j+1 > arg_register.size()-1;
        bool is_stack_float      = f   > arg_register_float.size()-1;
        bool is_next_stack_float = f+1 > arg_register_float.size()-1;
        if (is_stack) {
            reg1 = get_available_int_reg();
            reg2 = get_available_int_reg();
        } else if (is_next_stack) {
            reg1 = arg_register[j];
            reg2 = get_available_int_reg();
        } else {
            reg1 = arg_register[j];
            reg2 = arg_register[j+1];
        }
        if (is_stack_float) {
            reg3 = get_available_float_reg();
            reg4 = get_available_float_reg();
        } else if (is_next_stack_float) {
            reg3 = arg_register_float[f];
            reg4 = get_available_float_reg();
        } else {
            reg3 = arg_register_float[f];
            reg4 = arg_register_float[f+1];
        }
        if (args[i].type.info.kind == Kind::Struct) {
            auto strct = Struct::get_from_name(args[i].type.info.name, m_program->struct_storage);
            if (args[i].deref_count > 0) {
                if (get_ptr_count(args[i].type) == args[i].deref_count)
                    args[i].size = strct.size;
            }
            if (args[i].size <= 8) {
                if (strct.is_float_only && args[i].type.info.kind != Kind::Pointer) {
                    if (args[i].size < func.arguments[i].size) mov.append(0, reg3, 8);
                    mov_var(args[i], reg3);
                    j--;
                } else {
                    if (args[i].size < func.arguments[i].size) mov.append(0, reg1, 8);
                    mov_var(args[i], reg1);
                    f--;
                }
            } else if (args[i].size <= 16) {
                if (strct.is_float_only) {
                    size_t orig_size = args[i].size;
                    auto deref_count = args[i].deref_count;
                    if (deref_count == 0) {
                        if (args[i].size < func.arguments[i].size) {
                            mov.append(0, reg3, 8);
                            mov.append(0, reg4, 8);
                        }
                        args[i].size = 8;
                        mov_var(args[i], reg3);
                        args[i].size = orig_size-8;
                        args[i].offset -= 8;
                        if (args[i].size < 8) args[i].type = type_infos.at("float");
                        mov_var(args[i], reg4);
                    } else if (deref_count > 0) {
                        if (args[i].size < func.arguments[i].size) {
                            mov.append(0, reg3, 8);
                            mov.append(0, reg4, 8);
                        }
                        auto reg = get_available_int_reg();
                        args[i].deref_count = 0;
                        args[i].size = 8;
                        mov_var(args[i], reg);
                        mov.append(0, reg, reg3, 8);
                        if (orig_size-8 < 8)
                            movs.append(8, reg, reg4, orig_size-8);
                        else
                            mov.append(8, reg, reg4, orig_size-8);
                        free_int_reg(reg);
                    }
                    f++;
                    j--;
                } else {
                    size_t orig_size = args[i].size;
                    auto deref_count = args[i].deref_count;
                    if (deref_count == 0) {
                        if (args[i].size < func.arguments[i].size) {
                            mov.append(0, reg1, 8);
                            mov.append(0, reg2, 8);
                        }
                        args[i].size = 8;
                        mov_var(args[i], reg1);
                        args[i].size = orig_size-8;
                        args[i].offset -= 8;
                        mov_var(args[i], reg2);
                    } else if (deref_count > 0) {
                        if (args[i].size < func.arguments[i].size) {
                            mov.append(0, reg1, 8);
                            mov.append(0, reg2, 8);
                        }
                        auto reg = get_available_int_reg();
                        args[i].deref_count = 0;
                        mov_var(args[i], reg);
                        mov.append(0, reg, reg1, 8);
                        mov.append(8, reg, reg2, orig_size-8);
                        free_int_reg(reg);
                    }
                    f--;
                }
            } else {
                args[i].deref_count = -1;
                if (args[i].size < func.arguments[i].size) mov.append(0, reg1, 8);
                mov_var(args[i], reg1);
            }
        } else {
            if (args[i].type.info.kind == Kind::Float) {
                mov_var(args[i], reg3);
                cast_float_size(reg3, args[i].size, temp_float_size);
                j--;
            } else {
                f--;
                if (!func.c_variadic && args[i].size < func.arguments[i].size) mov.append(0, reg1, 8);
                mov_var(args[i], reg1);
                if (func.c_variadic) {
                    if (args[i].size == 1)
                        output.appendf("    movsbl {}, {}\n", reg1._8, reg1._32);
                    if (args[i].size == 2)
                        output.appendf("    movswl {}, {}\n", reg1._16, reg1._32);
                }
            }
        }
        if (is_stack) {
            mov.append(reg1, current_stack_offset, Rsp);
            if (args[i].size > 8) {
                mov.append(reg2, current_stack_offset+8, Rsp);
            }
            current_stack_offset += std::max(8, (int)args[i].size);
        }
        if (is_stack_float) {
            mov.append(reg3, current_stack_offset, Rsp);
            current_stack_offset += std::max(8, (int)args[i].size);
        }
        if (is_next_stack) {
            if (args[i].size > 8) {
                mov.append(reg2, current_stack_offset+8, Rsp);
                current_stack_offset += 8;
            }
        }
        if (is_next_stack_float) {
            if (args[i].size > 8) {
                mov.append(reg4, current_stack_offset+8, Rsp);
                current_stack_offset += 8;
            }
        }
        free_int_reg(reg1);
        free_int_reg(reg2);
        free_float_reg(reg3);
        free_float_reg(reg4);
    }

    if (func.c_variadic) {
        mov.append(f, Rax);
    }
    output.appendf("    call {}\n", func.name);
}
void gnu_asm::call_func(Func func, VariableStorage args) {
    //if (args.size() > std::size(arg_register)) TODO("ERROR: stack arguments not implemented");
    if (m_program->platform == Platform::Windows) {
        call_func_windows(func, args);
    } else if (m_program->platform == Platform::Linux) {
        call_func_linux(func, args);
    }
}
void gnu_asm::mov_member(Register src, Variable dest) {
    Variable current = dest;
    Variable* parent = dest.parent;
    size_t off = 0;
    Register reg = get_available_int_reg();
    if (parent == nullptr)
        off = current.offset;
    while (parent != nullptr) {
        parent = current.parent;
        if (parent == nullptr) {
            off = current.offset - off;
            break;
        }
        off += current.offset;
        if (parent->type.info.kind != Kind::Pointer) {
        } else {
            //mov_var(*parent->parent, reg);
            mov_member(*parent, reg);
            parent->deref_count = get_ptr_count(parent->type) - 1;
            deref(reg, parent->deref_count);
            //mov_var(current.offset, reg, reg);
            if (is_float_reg(src))
                movs.append(src, off, reg, dest.size);
            else 
                mov.append(src, off, reg, dest.size);
            free_int_reg(reg);
            return;
        }
        current = *current.parent;
    }
    if (dest.deref_count > 0) {
        dest.deref_count -= 1;
        mov.append(-off, Rbp, reg);
        deref(reg, dest.deref_count);
        if (is_float_reg(src))
            movs.append(src, 0, reg, dest.size);
        else 
            mov.append(src, 0, reg, dest.size);
    } else {
        if (is_float_reg(src))
            movs.append(src, -off, Rbp, dest.size);
        else 
            mov.append(src, -off, Rbp, dest.size);
    }
    free_int_reg(reg);
}
void gnu_asm::mov_member(Variable src, Register dest) {
    Variable current = src;
    Variable* parent = src.parent;
    size_t off = 0;
    if (parent == nullptr)
        off = current.offset;
    while (parent != nullptr) {
        parent = current.parent;
        if (parent == nullptr) {
            off = current.offset - off;
            break;
        }
        //mlog::println("curr {}, par {}", current.name, parent->name);
        off += current.offset;
        if (parent->type.info.kind != Kind::Pointer) {
        } else {
            //mlog::println("deref => curr {}, par {}", current.name, parent->name);
            auto reg = get_available_int_reg();
            mov_member(*parent, reg);
            parent->deref_count = get_ptr_count(parent->type) - 1;
            deref(reg, parent->deref_count);

            if (src.deref_count == -1) {
                lea.append(off, reg, dest);
            } else {
                if (is_float_reg(dest) && src.type.info.kind != Kind::Struct) {
                    movs.append(off, reg, dest, src.size);
                } else 
                    mov.append(off, reg, dest, src.size);
                if (src.deref_count > 0) {
                    deref(dest, src.deref_count);
                }
            }
            free_int_reg(reg);
            return;
        }
        current = *current.parent;
    }
    if (src.deref_count == -1) {
        lea.append(-off, Rbp, dest);
    } else {
        if (is_float_reg(dest) && src.type.info.kind != Kind::Struct) {
            movs.append(-off, Rbp, dest, src.size);
            if (src.deref_count > 0) {
                deref(dest, src.deref_count);
            }
        } else {
            mov.append(-off, Rbp, dest, src.size);
            if (src.deref_count > 0) {
                deref(dest, src.deref_count);
            }
        }
    }
}
void gnu_asm::mov_var(Variable src, Register dest) {
    //std::string_view& reg_name = REG_SIZE(dest, src.size);

    if (src.type.qualifiers & Qualifier::literal && src.type.info.id == TypeId::String)
        lea.append(src.name, Rip, dest);
        //TODO("add lea func");
        //output.appendf("    leaq {}(%rip), {}\n", src.name, reg_name);
    else if (src.type.qualifiers & Qualifier::literal && src.type.info.kind == Kind::Int)
        mov.append(src.Int_val, dest);
    else if (src.type.qualifiers & Qualifier::literal && src.type.info.kind == Kind::Float) {
        //auto reg = get_available_int_reg();
        if (is_float_reg(dest))
            movs.append(src.name, Rip, dest, src.size);
        else
            mov.append(src.name, Rip, dest, src.size);
        //mov.append(src.name, Rip, reg);
        //mov.append(reg, dest);
        //free_int_reg(reg);
    } else if (src.type.qualifiers & Qualifier::global) {
        if (src.deref_count == -1) {
            lea.append(src.name, Rip, dest);
        } else if (src.deref_count > 0) {
            mov.append(src.name, Rip, dest, src.size);
            deref(dest, src.deref_count);
        } else {
            mov.append(src.name, Rip, dest, src.size);
        }
    } else if (src.type.info.kind == Kind::Void)
        mov.append(0, dest);
    else if (src.parent != nullptr) {
        mov_member(src, dest);
    } else if (src.deref_count > 0) {
        mov.append(-src.offset, Rbp, dest, src.size);
        deref(dest, src.deref_count);
    } else if (src.deref_count == -1) {
        lea.append(-src.offset, Rbp, dest);
    } else if (is_float_reg(dest) && src.type.info.kind != Kind::Struct) {
        movs.append(-src.offset, Rbp, dest, src.size);
    } else
        mov.append(-src.offset, Rbp, dest, src.size);
}
void gnu_asm::mov_var(Register src, Variable dest) {
    if (dest.type.qualifiers & Qualifier::constant) {
        TODO("can't move into a constant");
    } else if (dest.type.qualifiers & Qualifier::literal) {
        TODO("can't mov into literals");
    } else if (dest.type.info.kind == Kind::Void) {
        TODO("can't mov into Void");
    } else if (dest.deref_count > 0) {
        Register reg = get_available_int_reg();
        dest.deref_count -= 1;
        mov_var(dest, reg);
        mov.append(src, 0, reg, dest.size);
        free_int_reg(reg);
    } else if (dest.type.qualifiers & Qualifier::global) {
        mov.append(src, dest.name, Rip);
    } else {
        if (dest.parent != nullptr) {
            mov_member(src, dest);
        } else {
            if (dest.deref_count > 0) {
                dest.deref_count -= 1;
                mov.append(-dest.offset, Rbp, Rax);
                deref(Rax, dest.deref_count);
                if (is_float_reg(src))
                    movs.append(src, 0, Rax, dest.size);
                else 
                    mov.append(src, 0, Rax, dest.size);
            } else {
                if (is_float_reg(src))
                    movs.append(src, -dest.offset, Rbp, dest.size);
                else 
                    mov.append(src, -dest.offset, Rbp, dest.size);
            }
        }
    }

}
void gnu_asm::mov_var(Variable src, Variable dest) {
    if (dest.type.qualifiers & Qualifier::literal)
        TODO("can't mov into literals");
    if (dest.type.qualifiers & Qualifier::constant)
        TODO("can't move into a constant");

    int64_t src_real_ptr_count = (get_ptr_count(src.type)-src.deref_count);
    int64_t dest_real_ptr_count = (get_ptr_count(dest.type)-dest.deref_count);

    if (src.type.qualifiers & Qualifier::literal && src.type.info.kind == Kind::Int && dest.parent == nullptr) {
        if (dest.type.info.kind == Kind::Struct)
            TODO(mlog::format("can't mov int literal into var of type {}", dest.type.info.name));
        if (dest.type.qualifiers & Qualifier::global)
            mov.append(src.Int_val, dest.name, Rip, dest.size);
        else 
            mov.append(src.Int_val, -dest.offset, Rbp, dest.size);
    } else if (src.type.info.kind == Kind::Struct || dest.type.info.kind == Kind::Struct) {
        if (src.type.info.id != dest.type.info.id) 
            TODO(mlog::format("error trying assigning different structers to each other, {} {}", src.type.info.name, dest.type.info.name));
        Struct strct{};
        bool found = false;
        for (const auto& strct_ : m_program->struct_storage) {
            if (strct_.name == src.type.info.name) {
                found = true;
                strct = strct_;
                break;
            }
        }
        if (!found) TODO(mlog::format("struct {} wasn't found", src.type.info.name));
        // TODO: bug found where if you have something like this.color = ... it will be offset(%Rbp) instead of
        //       moving this to to a register and taking offset from it

        if (dest.type.info.kind == Kind::Pointer) {
            auto reg1 = get_available_int_reg();
            auto reg2 = get_available_int_reg();
            
            if (src_real_ptr_count != dest_real_ptr_count)
                TODO(mlog::format("trying to move Variable {} into {}, but kind is not the same", src.name, dest.name));
            if (dest_real_ptr_count == 0) {
                output.appendf("    cld\n");
                dest.deref_count -= 1;
                mov_var(dest, Rdi);

                if (src.type.info.kind == Kind::Pointer) {
                    src.deref_count -= 1;
                    mov_var(src, Rsi);
                } else {
                    lea.append(-src.offset, Rbp, Rsi);
                }

                mov.append(strct.size, Rcx);
                output.appendf("    rep movsb\n");
            } else if (dest_real_ptr_count > 0) {
                mov_var(src, reg1);
                mov_var(reg1, dest);
            } else 
                TODO("trying to store into a non lvalue");
            free_int_reg(reg1);
            free_int_reg(reg2);
        } else {
            output.appendf("    cld\n");
            src.deref_count = get_ptr_count(src.type) - 1;
            dest.deref_count = get_ptr_count(dest.type) - 1;
            mov_var(src, Rsi);
            mov_var(dest, Rdi);
            mov.append(strct.size, Rcx);
            output.appendf("    rep movsb\n");
        }
    } else {
        if (dest.type.info.kind == Kind::Float && dest.type.info.id != src.type.info.id) {
            auto reg = get_available_float_reg();
            mov_var(src, reg);
            cast_float_size(reg, src.size, dest.size);
            mov_var(reg, dest);

            free_float_reg(reg);
        } else if (dest.type.info.kind == Kind::Float && src.type.info.kind == Kind::Float) {
            auto reg = get_available_float_reg();
            mov_var(src, reg);
            mov_var(reg, dest);
            free_float_reg(reg);

        } else {
            auto reg = get_available_int_reg();
            mov_var(src, reg);
            mov_var(reg, dest);
            free_int_reg(reg);
        }
    }
}
void gnu_asm::deref(Register reg, int64_t deref_count) {
    if (deref_count == -1) {
        lea.append(reg, reg);
        return;
    } 
    while (deref_count > 0) {
        mov.append(0, reg, reg);
        deref_count -= 1;
    }
}
void gnu_asm::cast_int_size(Register reg, size_t orig_size, size_t new_size) {
    if (orig_size == new_size) return;
    if (orig_size > new_size)
        output.appendf("    cvtsd2ss {}, {}\n", reg._64, reg._64);
    else if (orig_size < new_size)
        output.appendf("    cvtss2sd {}, {}\n", reg._64, reg._64);
}
void gnu_asm::cast_float_size(Register reg, size_t orig_size, size_t new_size) {
    if (orig_size == new_size) return;
    if (orig_size > new_size)
        output.appendf("    cvtsd2ss {}, {}\n", reg._64, reg._64);
    else if (orig_size < new_size)
        output.appendf("    cvtss2sd {}, {}\n", reg._64, reg._64);
}
void gnu_asm::function_prologue() {
    output.append("    pushq %rbp\n");
    output.append("    movq %rsp, %rbp\n");
    output.append("    pushq %rbx\n");
    output.append("    pushq %r13\n");
    output.append("    pushq %r14\n");
    output.append("    pushq %r15\n");
}
void gnu_asm::function_epilogue() {
    output.append("    popq %r15\n");
    output.append("    popq %r14\n");
    output.append("    popq %r13\n");
    output.append("    popq %rbx\n");
    output.append("    movq %rbp, %rsp\n");
    output.append("    popq %rbp\n");
}
