#include "template/rv32im_decoder.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// ABI názvy registrů x0..x31
static const char* const rv_regs[32] = {
    "zero","ra","sp","gp","tp","t0","t1","t2",
    "s0","s1","a0","a1","a2","a3","a4","a5",
    "a6","a7","s2","s3","s4","s5","s6","s7",
    "s8","s9","s10","s11","t3","t4","t5","t6"
};

int rv32im_decode(uint32_t instr, char* out, size_t out_len) {
    if (!out || out_len < 4) return 0;
    out[0] = '\0';

    uint8_t opcode = instr & 0x7F;
    uint8_t rd     = (instr >> 7)  & 0x1F;
    uint8_t funct3 = (instr >> 12) & 0x07;
    uint8_t rs1    = (instr >> 15) & 0x1F;
    uint8_t rs2    = (instr >> 20) & 0x1F;
    uint8_t funct7 = (instr >> 25) & 0x7F;

    // I-immediate (12b, sign-extended)
    int32_t imm_i = ((int32_t)instr) >> 20;
    // S-immediate (12b)
    int32_t imm_s = (((int32_t)instr >> 25) << 5) | ((instr >> 7) & 0x1F);
    // B-immediate (13b)
    int32_t imm_b = (((instr >> 31) & 1) << 12) | (((instr >> 7)  & 1) << 11) |
                    (((instr >> 25) & 0x3F) << 5) | (((instr >> 8) & 0xF) << 1);
    if (imm_b & 0x1000) imm_b |= 0xFFFFE000;
    // J-immediate (21b)
    int32_t imm_j = (((instr >> 31) & 1) << 20) | (((instr >> 12) & 0xFF) << 12) |
                    (((instr >> 20) & 1) << 11) | (((instr >> 21) & 0x3FF) << 1);
    if (imm_j & 0x100000) imm_j |= 0xFFE00000;
    // U-immediate (upper 20 bits)
    uint32_t imm_u = instr & 0xFFFFF000;

    switch (opcode) {
        case 0x33: { // OP (R-type + M extension)
            const char* mn = NULL;
            if (funct7 == 0x00) {
                switch (funct3) {
                    case 0x0: mn = "add";  break;
                    case 0x1: mn = "sll";  break;
                    case 0x2: mn = "slt";  break;
                    case 0x3: mn = "sltu"; break;
                    case 0x4: mn = "xor";  break;
                    case 0x5: mn = "srl";  break;
                    case 0x6: mn = "or";   break;
                    case 0x7: mn = "and";  break;
                }
            } else if (funct7 == 0x20) {
                switch (funct3) {
                    case 0x0: mn = "sub";  break;
                    case 0x5: mn = "sra";  break;
                }
            } else if (funct7 == 0x01) {
                switch (funct3) {
                    case 0x0: mn = "mul";    break;
                    case 0x1: mn = "mulh";   break;
                    case 0x2: mn = "mulhsu"; break;
                    case 0x3: mn = "mulhu";  break;
                    case 0x4: mn = "div";    break;
                    case 0x5: mn = "divu";   break;
                    case 0x6: mn = "rem";    break;
                    case 0x7: mn = "remu";   break;
                }
            }
            if (mn) {
                snprintf(out, out_len, "%s %s, %s, %s",
                         mn, rv_regs[rd], rv_regs[rs1], rv_regs[rs2]);
                return 4;
            }
            break;
        }

        case 0x13: { // OP-IMM
            switch (funct3) {
                case 0x0:
                    snprintf(out, out_len, "addi %s, %s, %d",
                             rv_regs[rd], rv_regs[rs1], imm_i);
                    return 4;
                case 0x1:
                    snprintf(out, out_len, "slli %s, %s, %u",
                             rv_regs[rd], rv_regs[rs1], (unsigned)rs2);
                    return 4;
                case 0x2:
                    snprintf(out, out_len, "slti %s, %s, %d",
                             rv_regs[rd], rv_regs[rs1], imm_i);
                    return 4;
                case 0x3:
                    snprintf(out, out_len, "sltiu %s, %s, %d",
                             rv_regs[rd], rv_regs[rs1], imm_i);
                    return 4;
                case 0x4:
                    snprintf(out, out_len, "xori %s, %s, %d",
                             rv_regs[rd], rv_regs[rs1], imm_i);
                    return 4;
                case 0x5:
                    if (funct7 == 0x20) {
                        snprintf(out, out_len, "srai %s, %s, %u",
                                 rv_regs[rd], rv_regs[rs1], (unsigned)rs2);
                    } else {
                        snprintf(out, out_len, "srli %s, %s, %u",
                                 rv_regs[rd], rv_regs[rs1], (unsigned)rs2);
                    }
                    return 4;
                case 0x6:
                    snprintf(out, out_len, "ori %s, %s, %d",
                             rv_regs[rd], rv_regs[rs1], imm_i);
                    return 4;
                case 0x7:
                    snprintf(out, out_len, "andi %s, %s, %d",
                             rv_regs[rd], rv_regs[rs1], imm_i);
                    return 4;
            }
            break;
        }

        case 0x03: { // LOAD
            const char* mn = NULL;
            switch (funct3) {
                case 0x0: mn = "lb";  break;
                case 0x1: mn = "lh";  break;
                case 0x2: mn = "lw";  break;
                case 0x4: mn = "lbu"; break;
                case 0x5: mn = "lhu"; break;
            }
            if (mn) {
                snprintf(out, out_len, "%s %s, %d(%s)",
                         mn, rv_regs[rd], imm_i, rv_regs[rs1]);
                return 4;
            }
            break;
        }

        case 0x23: { // STORE
            const char* mn = NULL;
            switch (funct3) {
                case 0x0: mn = "sb"; break;
                case 0x1: mn = "sh"; break;
                case 0x2: mn = "sw"; break;
            }
            if (mn) {
                snprintf(out, out_len, "%s %s, %d(%s)",
                         mn, rv_regs[rs2], imm_s, rv_regs[rs1]);
                return 4;
            }
            break;
        }

        case 0x63: { // BRANCH
            const char* mn = NULL;
            switch (funct3) {
                case 0x0: mn = "beq";  break;
                case 0x1: mn = "bne";  break;
                case 0x4: mn = "blt";  break;
                case 0x5: mn = "bge";  break;
                case 0x6: mn = "bltu"; break;
                case 0x7: mn = "bgeu"; break;
            }
            if (mn) {
                snprintf(out, out_len, "%s %s, %s, %d",
                         mn, rv_regs[rs1], rv_regs[rs2], imm_b);
                return 4;
            }
            break;
        }

        case 0x6F: // JAL
            snprintf(out, out_len, "jal %s, %d", rv_regs[rd], imm_j);
            return 4;

        case 0x67: // JALR
            if (funct3 == 0x0) {
                snprintf(out, out_len, "jalr %s, %d(%s)",
                         rv_regs[rd], imm_i, rv_regs[rs1]);
                return 4;
            }
            break;

        case 0x37: // LUI
            snprintf(out, out_len, "lui %s, 0x%05X",
                     rv_regs[rd], (unsigned)(imm_u >> 12));
            return 4;

        case 0x17: // AUIPC
            snprintf(out, out_len, "auipc %s, 0x%05X",
                     rv_regs[rd], (unsigned)(imm_u >> 12));
            return 4;

        case 0x73: // SYSTEM
            if (instr == 0x00000073) {
                snprintf(out, out_len, "ecall");
            } else if (instr == 0x00100073) {
                snprintf(out, out_len, "ebreak");
            } else if (funct3 == 0x0) {
                snprintf(out, out_len, "ecall/ebreak (0x%08X)", instr);
            } else {
                const char* csr_op =
                    (funct3 == 1) ? "csrrw"  :
                    (funct3 == 2) ? "csrrs"  :
                    (funct3 == 3) ? "csrrc"  :
                    (funct3 == 5) ? "csrrwi" :
                    (funct3 == 6) ? "csrrsi" :
                    (funct3 == 7) ? "csrrci" : "csr_?";
                snprintf(out, out_len, "%s %s, 0x%03X, %s",
                         csr_op, rv_regs[rd],
                         (unsigned)((instr >> 20) & 0xFFF), rv_regs[rs1]);
            }
            return 4;

        case 0x0F: // FENCE
            snprintf(out, out_len, "fence");
            return 4;
    }

    // Neznama instrukce – vrati .word
    snprintf(out, out_len, ".word 0x%08X", instr);
    return 4;
}