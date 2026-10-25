#include <stdint.h>
void rv32i_decode() {
    uint32_t instr = 0;//rv32i_fetch(cpu->pc - ROM_OFFSET);
    uint32_t old_pc = 0;//cpu->pc;  // ULOŽ SI STARÉ PC!
    uint8_t opcode = instr & 0x7F;          // 7 bitů LSB
    
    //int32_t imm_i = (int32_t)instr >> 20;  // immediate pro I-type (sign extend)
    int32_t imm_i = (int32_t)((instr >> 20) & 0xFFF);
    if (imm_i & 0x800) imm_i |= 0xFFFFF000;
    switch (opcode) {
        // ------------------------- R-type (ADD, SUB, AND, OR, XOR, SLT) -------------------------
        case 0x33: { // OP imediate (R-type)
            uint8_t rd = (instr >> 7) & 0x1F;       // 5 bitů
            uint8_t rs1 = (instr >> 15) & 0x1F;
            uint8_t rs2 = (instr >> 20) & 0x1F;
            uint8_t funct3 = (instr >> 12) & 0x7;   // 3 bity
            uint8_t funct7 = (instr >> 25) & 0x7F;  // 7 bitů (pro R-type)
            if (funct7 == 0x00 || funct7 == 0x20){
                switch (funct3) {
                    case 0x0:  // ADD nebo SUB podle funct7
                        if (funct7 == 0x00)
                            cpu->reg[rd] = cpu->reg[rs1] + cpu->reg[rs2];
                        else if (funct7 == 0x20)
                            cpu->reg[rd] = cpu->reg[rs1] - cpu->reg[rs2];
                        break;
                    case 0x1:  // SLL
                        cpu->reg[rd] = cpu->reg[rs1] << (cpu->reg[rs2] & 0x1F);
                        break;
                    case 0x2:  // SLT (signed)
                        cpu->reg[rd] = ((int32_t)cpu->reg[rs1] < (int32_t)cpu->reg[rs2]) ? 1 : 0;
                        break;
                    case 0x3:  // SLTU (unsigned)
                        cpu->reg[rd] = (cpu->reg[rs1] < cpu->reg[rs2]) ? 1 : 0;
                        break;
                    case 0x4:  // XOR
                        cpu->reg[rd] = cpu->reg[rs1] ^ cpu->reg[rs2];
                        break;
                    case 0x5:  // SRL (logical) nebo SRA (arithmetic) podle funct7
                        if (funct7 == 0x00)
                            cpu->reg[rd] = cpu->reg[rs1] >> (cpu->reg[rs2] & 0x1F);
                        else if (funct7 == 0x20)
                            cpu->reg[rd] = (uint32_t)((int32_t)cpu->reg[rs1] >> (cpu->reg[rs2] & 0x1F));
                        break;
                    case 0x6:  // OR
                        cpu->reg[rd] = cpu->reg[rs1] | cpu->reg[rs2];
                        break;
                    case 0x7:  // AND
                        cpu->reg[rd] = cpu->reg[rs1] & cpu->reg[rs2];
                        break;
                }
            }
            else if (funct7 == 0x01) {  // M extension (Multiplication and Division)
                uint32_t rs1_u = cpu->reg[rs1];
                uint32_t rs2_u = cpu->reg[rs2];
                int32_t rs1_s = (int32_t)rs1_u;
                int32_t rs2_s = (int32_t)rs2_u;
                        
                switch (funct3) {
                    case 0x0: // MUL - lower 32 bits
                        cpu->reg[rd] = rs1_u * rs2_u;
                        break;

                    case 0x1: // MULH - signed * signed, high 32 bits
                    {
                        // Rozklad na 16bitové poloviny
                        int16_t a_hi = rs1_s >> 16;
                        uint16_t a_lo = rs1_u & 0xFFFF;
                        int16_t b_hi = rs2_s >> 16;
                        uint16_t b_lo = rs2_u & 0xFFFF;

                        // (a_hi<<16 + a_lo) * (b_hi<<16 + b_lo) =
                        // a_hi*b_hi<<32 + (a_hi*b_lo + a_lo*b_hi)<<16 + a_lo*b_lo

                        uint32_t lo_lo = (uint32_t)a_lo * b_lo;           // a_lo * b_lo
                        uint32_t hi_lo = (uint32_t)a_hi * (int32_t)b_lo;  // a_hi * b_lo (signed)
                        uint32_t lo_hi = (uint32_t)a_lo * (int32_t)b_hi;  // a_lo * b_hi (signed)
                        int32_t hi_hi = (int32_t)a_hi * (int32_t)b_hi;    // a_hi * b_hi (signed)

                        // Sečtení s přenosy
                        uint32_t mid = hi_lo + lo_hi;
                        uint32_t low = lo_lo;
                        uint32_t carry = 0;

                        // Přičtení mid k low s přenosem
                        uint32_t sum = low + (mid << 16);
                        if (sum < low) carry = 1;

                        // Výsledek = hi_hi + (mid >> 16) + carry
                        uint32_t result_hi = hi_hi + (mid >> 16) + carry;
                        cpu->reg[rd] = result_hi;
                    }
                    break;

                    case 0x2: // MULHSU - signed * unsigned, high 32 bits
                    {
                        int16_t a_hi = rs1_s >> 16;
                        uint16_t a_lo = rs1_u & 0xFFFF;
                        uint16_t b_hi = rs2_u >> 16;
                        uint16_t b_lo = rs2_u & 0xFFFF;

                        uint32_t lo_lo = (uint32_t)a_lo * b_lo;
                        uint32_t hi_lo = (uint32_t)a_hi * b_lo;      // a_hi je signed, ale rozšíříme
                        uint32_t lo_hi = (uint32_t)a_lo * b_hi;
                        uint32_t hi_hi = (uint32_t)a_hi * b_hi;      // a_hi signed, b_hi unsigned

                        uint32_t mid = hi_lo + lo_hi;
                        uint32_t low = lo_lo;
                        uint32_t carry = 0;

                        uint32_t sum = low + (mid << 16);
                        if (sum < low) carry = 1;

                        uint32_t result_hi = hi_hi + (mid >> 16) + carry;
                        cpu->reg[rd] = result_hi;
                    }
                    break;

                    case 0x3: // MULHU - unsigned * unsigned, high 32 bits
                    {
                        uint16_t a_hi = rs1_u >> 16;
                        uint16_t a_lo = rs1_u & 0xFFFF;
                        uint16_t b_hi = rs2_u >> 16;
                        uint16_t b_lo = rs2_u & 0xFFFF;

                        uint32_t lo_lo = (uint32_t)a_lo * b_lo;
                        uint32_t hi_lo = (uint32_t)a_hi * b_lo;
                        uint32_t lo_hi = (uint32_t)a_lo * b_hi;
                        uint32_t hi_hi = (uint32_t)a_hi * b_hi;

                        uint32_t mid = hi_lo + lo_hi;
                        uint32_t low = lo_lo;
                        uint32_t carry = 0;

                        uint32_t sum = low + (mid << 16);
                        if (sum < low) carry = 1;

                        uint32_t result_hi = hi_hi + (mid >> 16) + carry;
                        cpu->reg[rd] = result_hi;
                    }
                    break;

                    case 0x4: // DIV - signed division
                    {
                        int32_t n = rs1_s;
                        int32_t d = rs2_s;

                        if (d == 0) {
                            cpu->reg[rd] = 0xFFFFFFFF;  // all bits set
                        } else if (n == 0x80000000 && d == -1) {
                            cpu->reg[rd] = 0x80000000;  // overflow
                        } else {
                            cpu->reg[rd] = n / d;
                        }
                    }
                    break;

                    case 0x5: // DIVU - unsigned division
                    {
                        uint32_t n = rs1_u;
                        uint32_t d = rs2_u;

                        if (d == 0) {
                            cpu->reg[rd] = 0xFFFFFFFF;
                        } else {
                            cpu->reg[rd] = n / d;
                        }
                    }
                    break;

                    case 0x6: // REM - signed remainder
                    {
                        int32_t n = rs1_s;
                        int32_t d = rs2_s;

                        if (d == 0) {
                            cpu->reg[rd] = n;
                        } else if (n == 0x80000000 && d == -1) {
                            cpu->reg[rd] = 0;
                        } else {
                            cpu->reg[rd] = n % d;
                        }
                    }
                    break;

                    case 0x7: // REMU - unsigned remainder
                    {
                        uint32_t n = rs1_u;
                        uint32_t d = rs2_u;

                        if (d == 0) {
                            cpu->reg[rd] = n;
                        } else {
                            cpu->reg[rd] = n % d;
                        }
                    }
                    break;
                }
            }
        }
        break;
        // ------------------------- I-type (ADDI, ANDI, ORI, XORI, SLLI, SRLI, SRAI, LW) -------------------------
        case 0x13: { //OP-IMMEDIATE
                uint8_t rd = (instr >> 7) & 0x1F;       // 5 bitů
                uint8_t rs1 = (instr >> 15) & 0x1F;
                uint8_t funct3 = (instr >> 12) & 0x7;   // 3 bity
                switch (funct3) {
                    case 0x0: cpu->reg[rd] = cpu->reg[rs1] + imm_i; 
                        break;  // ADDI
                    case 0x1: // SLLI
                        cpu->reg[rd] = cpu->reg[rs1] << (imm_i & 0x1F);
                        break;
                    case 0x2: // SLTI (signed)
                        cpu->reg[rd] = ((int32_t)cpu->reg[rs1] < (int32_t)imm_i) ? 1 : 0;
                        break;
                    case 0x3: // SLTIU (unsigned)
                        cpu->reg[rd] = (cpu->reg[rs1] < (uint32_t)imm_i) ? 1 : 0;
                        break;
                    case 0x4: // XORI
                        cpu->reg[rd] = cpu->reg[rs1] ^ imm_i;
                        break;
                    case 0x5: // SRLI / SRAI podle imm bitů (zjednodušeně předpokládáme SRLI)
                        cpu->reg[rd] = cpu->reg[rs1] >> (imm_i & 0x1F); //TODO: fix this
                        break;
                    case 0x6: // ORI
                        cpu->reg[rd] = cpu->reg[rs1] | imm_i;
                        break;
                    case 0x7: // ANDI
                        cpu->reg[rd] = cpu->reg[rs1] & imm_i;
                        break;
                }
            }
            break;
        // ------------------------- Load (LW, LH atd. jen LW pro kostru) -------------------------
        case 0x03: // LOAD
        {
            uint8_t rs1 = (instr >> 15) & 0x1F;
            uint8_t rd = (instr >> 7) & 0x1F;
            uint32_t load_addr = (cpu->reg[rs1] + imm_i);
            uint32_t val = 0;
            // Čtení z RAM (stack, proměnné)
            uint8_t funct3 = (instr >> 12) & 0x7;   // 3 bity
            switch (funct3) {
                case 0x0: { // LB
                    uint8_t byte = cpu->ram[load_addr];
                    cpu->reg[rd] = (int32_t)(int8_t)byte;
                    break;
                }
                case 0x1: { // LH
                    uint16_t half = cpu->ram[load_addr] | (cpu->ram[load_addr+1] << 8);
                    cpu->reg[rd] = (int32_t)(int16_t)half;
                    break;
                }
                case 0x2: { // LW
                    uint32_t word = (uint32_t)cpu->ram[load_addr] | 
                                    ((uint32_t)cpu->ram[load_addr+1] << 8) | 
                                    ((uint32_t)cpu->ram[load_addr+2] << 16) | 
                                    ((uint32_t)cpu->ram[load_addr+3] << 24);
                    cpu->reg[rd] = word;
                    break;
                }
                case 0x4: { // LBU
                    cpu->reg[rd] = cpu->ram[load_addr];
                    break;
                }
                case 0x5: { // LHU
                    cpu->reg[rd] = cpu->ram[load_addr] | (cpu->ram[load_addr+1] << 8);
                    break;
                }
            }
        }
        break;

// Store instrukce
        case 0x23: // STORE
            {
            uint8_t rs1 = (instr >> 15) & 0x1F;
            uint8_t rs2 = (instr >> 20) & 0x1F;
            uint8_t funct3 = (instr >> 12) & 0x7;   // 3 bity
            int32_t imm_s = ((instr >> 7) & 0x1F) | ((instr >> 25) << 5);
            if (imm_s & 0x800) imm_s |= 0xFFFFF000;     // Znaménkové rozšíření z 12 bitů
            uint32_t store_addr = (cpu->reg[rs1] + imm_s);
                switch (funct3) {
                    case 0x0: { // SB
                        cpu->ram[store_addr] = (uint8_t)cpu->reg[rs2];
                        break;
                    }
                    case 0x1: {  // SH - uložit little‑endian halfword
                        uint16_t val = (uint16_t)cpu->reg[rs2];
                        cpu->ram[store_addr]     = val & 0xFF;
                        cpu->ram[store_addr + 1] = (val >> 8) & 0xFF;
                        break;
                    }
                    case 0x2: { // SW - uložit little‑endian word
                        uint32_t val = cpu->reg[rs2];
                        cpu->ram[store_addr]     = val & 0xFF;
                        cpu->ram[store_addr + 1] = (val >> 8) & 0xFF;
                        cpu->ram[store_addr + 2] = (val >> 16) & 0xFF;
                        cpu->ram[store_addr + 3] = (val >> 24) & 0xFF;
                        break;   

                    }       
                    case 0x3: // SD pro RV64 – ignorovat
                        break;
                }
            
        }
        break;
        // ------------------------- Branch (BEQ, BNE, BLT, BGE, BLTU, BGEU) -------------------------
        case 0x63: // BRANCH
            {
                uint8_t rs1 = (instr >> 15) & 0x1F;
                uint8_t rs2 = (instr >> 20) & 0x1F;
                uint8_t funct3 = (instr >> 12) & 0x7;   // 3 bity
                // pro zjednodušení - make immediate z instr (skutečná by rozkousala bity)
                 int32_t offset = ((instr >> 31) << 12) |        // bit 12
                                 ((instr >> 25) & 0x3F) << 5 |  // bity 10:5
                                 ((instr >> 8) & 0xF) << 1 |    // bity 4:1
                                 ((instr >> 7) & 0x1) << 11;    // bit 11
                // Znaménkové rozšíření z 13 bitů
                if (offset & 0x1000) offset |= 0xFFFFF000;
                int32_t taken = 0;
                switch (funct3) {
                    case 0x0: if (cpu->reg[rs1] == cpu->reg[rs2]) taken = 1; break; // BEQ
                    case 0x1: if (cpu->reg[rs1] != cpu->reg[rs2]) taken = 1; break; // BNE
                    case 0x4: if ((int32_t)cpu->reg[rs1] < (int32_t)cpu->reg[rs2]) taken = 1; break; // BLT
                    case 0x5: if ((int32_t)cpu->reg[rs1] >= (int32_t)cpu->reg[rs2]) taken = 1; break; // BGE
                    case 0x6: if (cpu->reg[rs1] < cpu->reg[rs2]) taken = 1; break; // BLTU
                    case 0x7: if (cpu->reg[rs1] >= cpu->reg[rs2]) taken = 1; break; // BGEU
                }
                 if (taken) {
                    cpu->pc = old_pc + offset;  // ŽÁDNÉ -4!
                } else {
                    cpu->pc = old_pc + 4;
                }
            }
            break;
        // ------------------------- JAL (unconditional jump) -------------------------
        case 0x6F: // JAL
        {
            uint8_t rd = (instr >> 7) & 0x1F;
            int32_t offset = 0;
            offset |= ((uint32_t)((instr >> 31) & 0x1)) << 20;  // imm[20]
            offset |= ((uint32_t)((instr >> 21) & 0x3FF)) << 1; // imm[10:1]
            offset |= ((uint32_t)((instr >> 20) & 0x1)) << 11;  // imm[11]
            offset |= ((uint32_t)((instr >> 12) & 0xFF)) << 12; // imm[19:12]

            // Znaménkové rozšíření z 21 bitů
            if (offset & 0x100000)
                offset |= 0xFFF00000;

            cpu->reg[rd] = old_pc + 4;
            cpu->pc = old_pc + offset;
        }
        break;
        case 0x67: // JALR (funct3 = 0x0)
        {   
            uint8_t rd = (instr >> 7) & 0x1F;
            uint8_t rs1 = (instr >> 15) & 0x1F;
            // I-type formát: offset (imm) je v instr[31:20] (12 bitů)
            int32_t offset = (int32_t)((instr >> 20) & 0xFFF);
            // Znaménkové rozšíření z 12 bitů (už je v offsetu jako int32_t)
            // Ale jelikož jsme použili int32_t, je to v pořádku.
            // Alternativně: offset = (offset << 20) >> 20;
            // Cílová adresa: rs1 + offset, ale nejnižší bit vynulovaný
            uint32_t target = (cpu->reg[rs1] + offset) & ~1;
            // Návratová adresa (ulož do rd)
            cpu->reg[rd] = old_pc + 4;
            cpu->pc = target; // Skok na cílovou adresu
        }
        break;

 
        // ------------------------- LUI (Load Upper Immediate) -------------------------
        case 0x37: { // LUI
            uint8_t rd = (instr >> 7) & 0x1F;
            cpu->reg[rd] = (instr & 0xFFFFF000);
            cpu->pc = old_pc + 4;
            break;
        }
        // ------------------------- AUIPC (Add Upper Immediate to PC) -------------------------
        case 0x17: { // AUIPC
            uint8_t rd = (instr >> 7) & 0x1F;
            cpu->reg[rd] = old_pc + (instr & 0xFFFFF000);  // TADY PC, NE old_pc?
            cpu->pc = old_pc + 4;
            break;
        }
        default:
            print("neznama instrukce: ");
            printHex(opcode);
            println(" ");
            while (1);
    }
}