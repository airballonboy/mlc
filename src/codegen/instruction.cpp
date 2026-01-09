#include <cassert>
#include "codegen/instruction.h"
#include "tools/format.h"

#define REG_SIZE(REG, SIZE)   (SIZE) == 8 ? (REG)._64 : (SIZE) == 4 ? (REG)._32 : (SIZE) == 2 ? (REG)._16 : (REG)._8 
#define INST_SIZE(INST, SUFF, SIZE) (SIZE) == 8 ? INST+SUFF._64 : (SIZE) == 4 ? INST + SUFF._32 : (SIZE) == 2 ? INST+SUFF._16 : INST+SUFF._8 

void AsmInstruction::append(int64_t offset, Register src, Register dest, size_t size) {
    if (offset == 0) {
        m_output.appendf("    {} ({}), {}\n",
                           INST_SIZE(m_instName, m_instSuffixs, size),
                           REG_SIZE(src, 8),
                           REG_SIZE(dest, size)
        );
    } else { 
        m_output.appendf("    {} {}({}), {}\n",
                           INST_SIZE(m_instName, m_instSuffixs, size),
                           offset, REG_SIZE(src, 8),
                           REG_SIZE(dest, size)
        );
    }
}
void AsmInstruction::append(Register src, int64_t offset, Register dest, size_t size) {
    if (offset == 0) {
        m_output.appendf("    {} {}, ({})\n",
                           INST_SIZE(m_instName, m_instSuffixs, size),
                           REG_SIZE(src, size),
                           REG_SIZE(dest, 8)
        );
    } else {
        m_output.appendf("    {} {}, {}({})\n",
                           INST_SIZE(m_instName, m_instSuffixs, size),
                           REG_SIZE(src, size),
                           offset, REG_SIZE(dest, 8)
        );
    }
}
void AsmInstruction::append(Register src, Register dest, size_t size) {
    m_output.appendf("    {} {}, {}\n",
                       INST_SIZE(m_instName, m_instSuffixs, size),
                       REG_SIZE(src, size),
                       REG_SIZE(dest, size)
    );
}
void AsmInstruction::append(int64_t int_value, Register dest, size_t size) {
    m_output.appendf("    {} $0x{:x}, {}\n",
                       INST_SIZE(m_instName, m_instSuffixs, size),
                       int_value,
                       REG_SIZE(dest, size)
    );
}
void AsmInstruction::append(int64_t  int_value, std::string label, Register dest, size_t size) {
    m_output.appendf("    {} $0x{:x}, {}({})\n",
                       INST_SIZE(m_instName, m_instSuffixs, size),
                       int_value,
                       label,
                       REG_SIZE(dest, 8)
    );
}
void AsmInstruction::append(int64_t int_value, int64_t offset, Register dest, size_t size) {
    if (offset == 0) {
        m_output.appendf("    {} $0x{:x}, ({})\n",
                           INST_SIZE(m_instName, m_instSuffixs, size),
                           int_value,
                           REG_SIZE(dest, 8)
        );
    } else {
        m_output.appendf("    {} $0x{:x}, {}({})\n",
                           INST_SIZE(m_instName, m_instSuffixs, size),
                           int_value,
                           offset, REG_SIZE(dest, 8)
        );
    }
}
void AsmInstruction::append(std::string global_label, Register src, Register dest, size_t size) {
    assert(size <= 8);
    m_output.appendf("    {} {}({}), {}\n",
                       INST_SIZE(m_instName, m_instSuffixs, size),
                       global_label, REG_SIZE(src, 8),
                       REG_SIZE(dest, size)
    );
}
void AsmInstruction::append(Register src, std::string global_label, Register dest, size_t size) {
    assert(size <= 8);
    m_output.appendf("    {} {}, {}({})\n",
                       INST_SIZE(m_instName, m_instSuffixs, size),
                       REG_SIZE(src, size),
                       global_label, REG_SIZE(dest, 8)
    );
}
void AsmInstruction::append(std::string global_label, Register src, Register dest) {
    append(global_label, src, dest, 8);
}
void AsmInstruction::append(Register src, std::string global_label, Register dest) {
    append(src, global_label, dest, 8);
}
void AsmInstruction::append(int64_t int_value, Register dest) {
    append(int_value, dest, 8);
}
void AsmInstruction::append(int64_t int_value, int64_t offset, Register dest) {
    append(int_value, offset, dest, 8);
}
void AsmInstruction::append(int64_t int_value, std::string label, Register dest) {
    append(int_value, label, dest, 8);
}
void AsmInstruction::append(int64_t offset, Register src, Register dest) {
    append(offset, src, dest, 8);
}
void AsmInstruction::append(Register src, int64_t offset, Register dest) {
    append(src, offset, dest, 8);
}
void AsmInstruction::append(Register src, Register dest) {
    append(src, dest, 8);
}
