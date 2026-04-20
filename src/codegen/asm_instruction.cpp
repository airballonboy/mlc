#include "codegen/asm_instruction.h"
#include "ast/mem/memory.h"
#include "tools/format.h"
#include "tools/logger.h"
#include <cassert>
#include <cstdint>

#define REG_SIZE(REG, SIZE)   (SIZE) == 8 ? (REG)._64 : (SIZE) == 4 ? (REG)._32 : (SIZE) == 2 ? (REG)._16 : (REG)._8 
#define INST_SIZE(INST, SUFF, SIZE) (SIZE) == 8 ? INST+SUFF._64 : (SIZE) == 4 ? INST + SUFF._32 : (SIZE) == 2 ? INST+SUFF._16 : INST+SUFF._8 

void AsmInstruction::append() {
    m_output->appendf("    {}\n", INST_SIZE(m_instName, m_instSuffixs, 8));
}
void AsmInstruction::append(Memory src, size_t size) {
    switch (src.asm_mem.type) {
    case AsmType::Reg:
        append(src.asm_mem.reg, size);
        break;
    case AsmType::OffReg:
        m_output->appendf("    {} {}({})\n",
                          INST_SIZE(m_instName, m_instSuffixs, 8),
                          src.asm_mem.off,
                          REG_SIZE(src.asm_mem.off_reg, size)                          
        );
        break;
    case AsmType::ArrayIndex:
        TODO("implement array index");
        break;
    case AsmType::Global:
        m_output->appendf("    {} {}({})\n",
                          INST_SIZE(m_instName, m_instSuffixs, 8),
                          src.asm_mem.label,
                          REG_SIZE(src.asm_mem.label_reg, size)                          
        );
        break;
    case AsmType::None:
        TODO("invalid Memory type");
    }
}
void AsmInstruction::append(Memory src, Memory dest, size_t size) {
    if (src.asm_mem.type == AsmType::Reg) {
        switch (dest.asm_mem.type) {
        case AsmType::Reg:
            if (src.asm_mem.reg._64 == dest.asm_mem.reg._64) break;
            append(src.asm_mem.reg, dest.asm_mem.reg, size);
            break;
        case AsmType::OffReg:
            append(src.asm_mem.reg, dest.asm_mem.off, dest.asm_mem.off_reg, size);
            break;
        case AsmType::ArrayIndex:
            TODO("implement array index");
            break;
        case AsmType::Global:
            append(src.asm_mem.reg, dest.asm_mem.label, dest.asm_mem.label_reg, size);
            break;
        case AsmType::None:
            TODO("invalid Memory type");
        }
    } else if (src.asm_mem.type == AsmType::OffReg) {
        switch (dest.asm_mem.type) {
        case AsmType::Reg:
            append(src.asm_mem.off, src.asm_mem.off_reg, dest.asm_mem.reg, size);
            break;
        case AsmType::OffReg:
        case AsmType::ArrayIndex:
        case AsmType::Global:
        case AsmType::None:
            TODO("invalid Memory type");
            break;
        }
    } else if (src.asm_mem.type == AsmType::Global) {
        switch (dest.asm_mem.type) {
        case AsmType::Reg:
            append(src.asm_mem.label, src.asm_mem.label_reg, dest.asm_mem.reg, size);
            break;
        case AsmType::OffReg:
        case AsmType::ArrayIndex:
        case AsmType::Global:
        case AsmType::None:
            TODO("invalid Memory type");
            break;
        }
    } else {
        TODO("invalid Memory type");
    }
}
void AsmInstruction::append(Register reg, size_t size) {
    m_output->appendf("    {} {}\n",
                       INST_SIZE(m_instName, m_instSuffixs, size),
                       REG_SIZE(reg, size)
    );
}
void AsmInstruction::append(int64_t offset, Register src, Register dest, size_t size) {
    if (offset == 0) {
        m_output->appendf("    {} ({}), {}\n",
                           INST_SIZE(m_instName, m_instSuffixs, size),
                           REG_SIZE(src, 8),
                           REG_SIZE(dest, size)
        );
    } else { 
        m_output->appendf("    {} {}({}), {}\n",
                           INST_SIZE(m_instName, m_instSuffixs, size),
                           offset, REG_SIZE(src, 8),
                           REG_SIZE(dest, size)
        );
    }
}
void AsmInstruction::append(Register src, int64_t offset, Register dest, size_t size) {
    if (offset == 0) {
        m_output->appendf("    {} {}, ({})\n",
                           INST_SIZE(m_instName, m_instSuffixs, size),
                           REG_SIZE(src, size),
                           REG_SIZE(dest, 8)
        );
    } else {
        m_output->appendf("    {} {}, {}({})\n",
                           INST_SIZE(m_instName, m_instSuffixs, size),
                           REG_SIZE(src, size),
                           offset, REG_SIZE(dest, 8)
        );
    }
}
void AsmInstruction::append(Register src, Register dest, size_t size) {
    m_output->appendf("    {} {}, {}\n",
                       INST_SIZE(m_instName, m_instSuffixs, size),
                       REG_SIZE(src, size),
                       REG_SIZE(dest, size)
    );
}
void AsmInstruction::append(int64_t int_value, Register dest, size_t size) {
    m_output->appendf("    {} $0x{:x}, {}\n",
                       INST_SIZE(m_instName, m_instSuffixs, size),
                       static_cast<uint64_t>(int_value),
                       REG_SIZE(dest, size)
    );
}
void AsmInstruction::append(int64_t  int_value, std::string label, Register dest, size_t size) {
    m_output->appendf("    {} $0x{:x}, {}({})\n",
                       INST_SIZE(m_instName, m_instSuffixs, size),
                       static_cast<uint64_t>(int_value),
                       label,
                       REG_SIZE(dest, 8)
    );
}
void AsmInstruction::append(int64_t int_value, int64_t offset, Register dest, size_t size) {
    if (offset == 0) {
        m_output->appendf("    {} $0x{:x}, ({})\n",
                           INST_SIZE(m_instName, m_instSuffixs, size),
                           static_cast<uint64_t>(int_value),
                           REG_SIZE(dest, 8)
        );
    } else {
        m_output->appendf("    {} $0x{:x}, {}({})\n",
                           INST_SIZE(m_instName, m_instSuffixs, size),
                           static_cast<uint64_t>(int_value),
                           offset, REG_SIZE(dest, 8)
        );
    }
}
void AsmInstruction::append(std::string global_label, Register src, Register dest, size_t size) {
    assert(size <= 8);
    m_output->appendf("    {} {}({}), {}\n",
                       INST_SIZE(m_instName, m_instSuffixs, size),
                       global_label, REG_SIZE(src, 8),
                       REG_SIZE(dest, size)
    );
}
void AsmInstruction::append(Register src, std::string global_label, Register dest, size_t size) {
    assert(size <= 8);
    m_output->appendf("    {} {}, {}({})\n",
                       INST_SIZE(m_instName, m_instSuffixs, size),
                       REG_SIZE(src, size),
                       global_label, REG_SIZE(dest, 8)
    );
}

Memory mem_off(int64_t off, Register reg, Type t) {
    return {.mem_type = MemType::Asm, .type = t, .asm_mem = {.type = AsmType::OffReg, .off = off, .off_reg = reg}};
}
Memory mem_reg(Register reg, Type t) {
    return {.mem_type = MemType::Asm, .type = t, .asm_mem = {.type = AsmType::Reg, .reg = reg}};
}
Memory mem_2reg(Register reg1, Register reg2, Type t) {
    return {.mem_type = MemType::Asm, .type = t, .asm_mem = {.type = AsmType::Reg, .reg1 = reg1, .reg2 = reg2}};
}
Memory mem_global(const char* label, Register reg, Type t) {
    return {.mem_type = MemType::Asm, .type = t, .asm_mem = {.type = AsmType::Global, .label = (char*)label, .label_reg = reg}};
}
Memory mem_array(int64_t disp, Register base, Register index, size_t scale, Type t) {
    return {.mem_type = MemType::Asm, .type = t, .asm_mem = {.type = AsmType::ArrayIndex, .disp = disp, .base = base, .index = index, .scale = scale}};
}
