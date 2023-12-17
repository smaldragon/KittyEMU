#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct cpu {
    uint16_t PC; // Program Counter
    uint8_t A; // Accumulator
    uint8_t X; // Index X
    uint8_t Y; // Index Y
    uint8_t S; // Stack Pointer
    uint8_t P; // Program Flags
    
    uint16_t DL; // Data Latch
    uint16_t I;   // Instruction Latch
    uint8_t C;   // Cycle Counter
    uint16_t TARGET;
    uint8_t MODE;
    uint8_t RMW;
    
    uint8_t RESET;
    uint8_t IRQ;
    uint8_t NMI;
    uint8_t IN_INTERRUPT;

    uint8_t PAGEFLIP;
} CPU;

typedef enum mem {
    WRITE,READ
} MEM;

typedef struct access {
    //enums for 8 bit registers
    MEM type;
    uint16_t address;
    uint8_t value;
} ACCESS;


typedef struct cpu cpu;



typedef enum mode {
    A,
    IAX,
    AX,
    AY,
    IA,
    ACC,
    IMD,
    IMP,
    R,
    SH,
    SL,
    Z,
    IZX,
    ZX,
    ZY,
    IZ,
    IZY,
    AJSR,
    ARTS,
    ARTI,
    INT,
    HLD,
    LAST_MODE,
} MODE;


typedef struct opcode {
    char name[3];
    
} OPCODE;

typedef enum FLAGS {
    FLAG_CARRY,
    FLAG_ZERO,
    FLAG_IRQ_DISABLE,
    FLAG_DECIMAL,
    FLAG_IRQ_TYPE,
    FLAG_ONE,
    FLAG_OVERFLOW,
    FLAG_NEGATIVE,
} FLAGS;

typedef enum opcodes {
    NO_OPCODE,
    ADC,AND,ASL,BBR,BBS,BCC,BCS,BEQ,BIT,BMI,
    BNE,BPL,BRA,BRK,BVC,BVS,CLC,CLD,CLI,CLV,
    CMP,CPX,CPY,DEC,DEX,DEY,EOR,INC,INX,INY,
    JMP,JSR,LDA,LDX,LDY,LSR,NOP,ORA,PHA,PHP,
    PHX,PHY,PLA,PLP,PLX,PLY,RMB,ROL,ROR,RTI,
    RTS,SBC,SEC,SED,SEI,SMB,STA,STP,STX,STY,
    STZ,TAX,TAY,TRB,TSB,TSX,TXA,TXS,TYA,WAI,
    INA,DEA,
    LAST_OPCODE,
} OPCODES;

uint8_t (*phase_1[LAST_OPCODE]) (CPU *cpu, ACCESS *result);
uint8_t (*phase_2[LAST_OPCODE]) (CPU *cpu, uint8_t operand);
uint8_t (*add_mode_1[LAST_MODE]) (CPU *cpu, ACCESS *result);
uint8_t (*add_mode_2[LAST_OPCODE]) (CPU *cpu, uint8_t operand);

int cpu_opend(CPU *cpu) {
    cpu->C = 0;
    cpu->P |= 0x20;
    cpu->P &= 0xEF;
}

uint8_t op_codes[256] = {
//     0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
/*0*/ BRK,ORA, 0 , 0 ,TSB,ORA,ASL,RMB,PHP,ORA,ASL, 0 ,TSB,ORA,ASL,BBR,
/*1*/ BPL,ORA,ORA, 0 ,TRB,ORA,ASL,RMB,CLC,ORA,INA, 0 ,TRB,ORA,ASL,BBR,
/*2*/ JSR,AND, 0 , 0 ,BIT,AND,ROL,RMB,PLP,AND,ROL, 0 ,BIT,AND,ROL,BBR,
/*3*/ BMI,AND,AND, 0 ,BIT,AND,ROL,RMB,SEC,AND,DEA, 0 ,BIT,AND,ROL,BBR,
/*4*/ RTI,EOR, 0 , 0 , 0 ,EOR,LSR,RMB,PHA,EOR,LSR, 0 ,JMP,EOR,LSR,BBR,
/*5*/ BVC,EOR,EOR, 0 , 0 ,EOR,LSR,RMB,CLI,EOR,PHY, 0 , 0 ,EOR,LSR,BBR,
/*6*/ RTS,ADC, 0 , 0 ,STZ,ADC,ROR,RMB,PLA,ADC,ROR, 0 ,JMP,ADC,ROR,BBR,
/*7*/ BVS,ADC,ADC, 0 ,STZ,ADC,ROR,RMB,SEI,ADC,PLY, 0 ,JMP,ADC,ROR,BBR,
/*8*/ BRA,STA, 0 , 0 ,STY,STA,STX,SMB,DEY,BIT,TXA, 0 ,STY,STA,STX,BBS,
/*9*/ BCC,STA,STA, 0 ,STY,STA,STX,SMB,TYA,STA,TXS, 0 ,STZ,STA,STZ,BBS,
/*A*/ LDY,LDA,LDX, 0 ,LDY,LDA,LDX,SMB,TAY,LDA,TAX, 0 ,LDY,LDA,LDX,BBS,
/*B*/ BCS,LDA,LDA, 0 ,LDY,LDA,LDX,SMB,CLV,LDA,TSX, 0 ,LDY,LDA,LDX,BBS,
/*C*/ CPY,CMP, 0 , 0 ,CPY,CMP,DEC,SMB,INY,CMP,DEX,WAI,CPY,CMP,DEC,BBS,
/*D*/ BNE,CMP,CMP, 0 , 0 ,CMP,DEC,SMB,CLD,CMP,PHX,STP, 0 ,CMP,DEC,BBS,
/*E*/ CPX,SBC, 0 , 0 ,CPX,SBC,INC,SMB,INX,SBC,NOP, 0 ,CPX,SBC,INC,BBS,
/*F*/ BEQ,SBC,SBC, 0 , 0 ,SBC,INC,SMB,SED,SBC,PLX, 0 , 0 ,SBC,INC,BBS,
};

uint8_t op_modes[256] = {
//    0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
/*0*/INT ,IZX,IMP,IMP, Z , Z , Z , Z , SH ,IMD,ACC,IMP, A , A , A , R ,
/*1*/ R  ,IZY, IZ,IMP, Z , ZX, ZX, Z ,IMP, AY,ACC,IMP, A , AX, AX, R ,
/*2*/AJSR,IZX,IMP,IMP, Z , Z , Z , Z ,SL ,IMD,ACC,IMP, A , A , A , R ,
/*3*/ R  ,IZY, IZ,IMP, ZX, ZX, ZX, Z ,IMP, AY,ACC,IMP, AX, AX, AX, R ,
/*4*/ARTI,IZX,IMP,IMP,IMP, Z , Z , Z ,SH ,IMD,ACC,IMP, A , A , A , R ,
/*5*/ R  ,IZY, IZ,IMP,IMP, ZX, ZX, Z ,IMP, AY, SH,IMP,IMP, AX, AX, R ,
/*6*/ARTS,IZX,IMP,IMP, Z , Z , Z , Z ,SL ,IMD,ACC,IMP, IA, A , A , R ,
/*7*/ R  ,IZY, IZ,IMP, ZX, ZX, ZX, Z ,IMP, AY, SL,IMP,IAX, AX, AX, R ,
/*8*/ R  ,IZX,IMP,IMP, Z , Z , Z , Z ,IMP,IMD,IMP,IMP, A , A , A , R ,
/*9*/ R  ,IZY, IZ,IMP, ZX, ZX, ZY, Z ,IMP, AY,IMP,IMP, A , AX, AX, R ,
/*A*/IMD ,IZX,IMD,IMP, Z , Z , Z , Z ,IMP,IMD,IMP,IMP, A , A , A , R ,
/*B*/ R  ,IZY, IZ,IMP, ZX, ZX, ZY, Z ,IMP, AY,IMP,IMP, AX, AX, AY, R ,
/*C*/IMD ,IZX,IMP,IMP, Z , Z , Z , Z ,IMP,IMD,IMP,IMP, A , A , A , R ,
/*D*/ R  ,IZY, IZ,IMP,IMP, ZX, ZX, Z ,IMP, AY, SH,IMP,IMP, AX, AX, R ,
/*E*/IMD ,IZX,IMP,IMP, Z , Z , Z , Z ,IMP,IMD,IMP,IMP, A , A , A , R ,
/*F*/ R  ,IZY, IZ,IMP,IMP, ZX, ZX, Z ,IMP, AY, SL,IMP,IMP, AX, AX, R ,
};

// Whether an instruction is a read-modify-write
uint8_t op_rmw[256] = {
//    0 1 2 3 4 5 6 7 8 9 A B C D E F
/*0*/ 0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
/*1*/ 0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
/*2*/ 0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
/*3*/ 0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
/*4*/ 0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
/*5*/ 0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
/*6*/ 0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
/*7*/ 0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
/*8*/ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*9*/ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*A*/ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*B*/ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*C*/ 0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
/*D*/ 0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
/*E*/ 0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
/*F*/ 0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0
};

uint8_t calc_NZ(uint8_t flag, uint8_t operand) {
    if (operand == 0) {
        flag |= 0x02;
    } else {
        flag &= 0xFD;
    }
    
    if (operand & 0x80) {
        flag |= 0x80;
    } else {
        flag &= 0x7F;
    }
    
    return flag;
}

// NOP
uint8_t nop_p1(CPU *cpu,ACCESS *result) {
    result->type = READ; result->address = cpu->PC;
}
uint8_t nop_p2(CPU *cpu, uint8_t operand) {
    return operand;
}

uint8_t adc_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t adc_p2(CPU *cpu, uint8_t operand) {
    uint8_t acc = cpu->A;
    uint8_t add = operand + ((cpu->P >> FLAG_CARRY) & 1);
    uint16_t result = cpu->A + add;
    cpu->A = result & 0xFF;
    
    // Set Flags
    cpu->P = cpu->P & 0x3C;
     
    uint8_t M = acc & 0x80;
    uint8_t N = operand & 0x80;
    cpu->P |= ( ((M^cpu->A)&(N^cpu->A)&0x80) != 0)<< FLAG_OVERFLOW;
    
    cpu->P = calc_NZ(cpu->P,cpu->A);
    cpu->P |= (result > 0xFF) << FLAG_CARRY;
    
    return operand;
}
uint8_t sbc_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t sbc_p2(CPU *cpu, uint8_t operand) {
    uint8_t acc = cpu->A;
    uint8_t sub = operand + (! ((cpu->P >> FLAG_CARRY) & 1 ));
    cpu->A = cpu->A - sub;
    
    // Set Flags
    cpu->P = cpu->P & 0x3C;
    
    uint8_t M = acc & 0x80;
    uint8_t N = (255-sub) & 0x80;
    cpu->P |= ( ((M^cpu->A)&(N^cpu->A)&0x80) != 0)<< FLAG_OVERFLOW;
    
    cpu->P = calc_NZ(cpu->P,cpu->A);
    cpu->P |= (cpu->A < acc) << FLAG_CARRY;
    
    return operand;
}

/*============================
    BOOLEAN MATH
 ===========================*/
uint8_t and_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t and_p2(CPU *cpu, uint8_t operand) {
    cpu->A = cpu->A & operand;
    cpu->P = calc_NZ(cpu->P,cpu->A);
    
    return operand;
}
uint8_t eor_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t eor_p2(CPU *cpu, uint8_t operand) {
    cpu->A = cpu->A ^ operand;
    cpu->P = calc_NZ(cpu->P,cpu->A);
    
    return operand;
}
uint8_t ora_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t ora_p2(CPU *cpu, uint8_t operand) {
    cpu->A = cpu->A | operand;
    cpu->P = calc_NZ(cpu->P,cpu->A);
    
    return operand;
}
/*============================
    BIT TESTING
 ===========================*/
uint8_t bit_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t bit_p2(CPU *cpu, uint8_t operand) {
    uint8_t zero = (operand & cpu->A) == 0;
    cpu->P = (cpu->P & 0x3D) + (zero << 1) + (operand & 0xC0);
    
    return operand;
}

uint8_t cmp_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t cmp_p2(CPU *cpu, uint8_t operand) {
    uint8_t acc_1 = cpu->A;
    uint8_t acc_2 = cpu->A - operand;
    
    // Set Flags
    cpu->P = cpu->P & 0x7C;
    //cpu->P |= (acc_1 > 127 && acc_2 <= 127) << FLAG_OVERFLOW;
    cpu->P |= (acc_1 >= operand) << FLAG_CARRY;
    //printf("CPM MOMENT %X %X",acc_1,acc_2);
    cpu->P = calc_NZ(cpu->P,acc_2);
    
    return operand;
}

uint8_t cpx_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t cpx_p2(CPU *cpu, uint8_t operand) {
    uint8_t acc_1 = cpu->X;
    uint8_t acc_2 = cpu->X - operand;
    
    // Set Flags
    cpu->P = cpu->P & 0x7C;
    
    
    //printf("CPX MOMENT %X %X",acc_1,acc_2);
    //cpu->P |= (acc_1 > 127 && acc_2 <= 127) << FLAG_OVERFLOW;
    cpu->P |= (acc_1 >= operand) << FLAG_CARRY;
    cpu->P = calc_NZ(cpu->P,acc_2);
    
    return operand;
}
uint8_t cpy_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t cpy_p2(CPU *cpu, uint8_t operand) {
    uint8_t acc_1 = cpu->Y;
    uint8_t acc_2 = cpu->Y - operand;
    
    // Set Flags
    cpu->P = cpu->P & 0x7C;
    //cpu->P |= (acc_1 > 127 && acc_2 <= 127) << FLAG_OVERFLOW;
    cpu->P |= (acc_1 >= operand) << FLAG_CARRY;
    cpu->P = calc_NZ(cpu->P,acc_2);
    
    return operand;
}
/*============================
    BIT SHIFTING
 ===========================*/
// Rotate Left
uint8_t rol_p1(CPU *cpu,ACCESS *result) {
    if (cpu->MODE == ACC) {
        result->type = READ;
        result->address = cpu->PC;
    } else {
        result->type = READ;
        result->address = cpu->TARGET;
    }
}
uint8_t rol_p2(CPU *cpu, uint8_t operand) {

    uint8_t old_carry = cpu->P & 1;
    uint8_t new_carry;
    if (cpu->MODE == ACC) {
        new_carry = (cpu->A & 0x80) >> 7;
        cpu->A = (cpu->A << 1) + old_carry;
        operand = cpu->A;
    } else {
        new_carry = (operand & 0x80) >> 7;
        operand = (operand << 1) + old_carry;
    }
    
    cpu->P &= 0x7C;
    cpu->P += new_carry;
    cpu->P = calc_NZ(cpu->P,operand);
    
    return operand;
}
// Shift Left
uint8_t asl_p1(CPU *cpu,ACCESS *result) {
    if (cpu->MODE == ACC) {
        result->type = READ;
        result->address = cpu->PC;
    } else {
        result->type = READ;
        result->address = cpu->TARGET;
    }
}
uint8_t asl_p2(CPU *cpu, uint8_t operand) {
    uint8_t new_carry;
    if (cpu->MODE == ACC) {
        new_carry = (cpu->A & 0x80) >> 7;
        cpu->A = cpu->A << 1;
        operand = cpu->A;
    } else {
        new_carry = (operand & 0x80) >> 7;
        operand = operand << 1;
    }
    
    cpu->P &= 0x7C;
    cpu->P += new_carry;
    cpu->P = calc_NZ(cpu->P,operand);
    
    return operand;
}
// Rotate Right
uint8_t ror_p1(CPU *cpu,ACCESS *result) {
    if (cpu->MODE == ACC) {
        result->type = READ;
        result->address = cpu->PC;
    } else {
        result->type = READ;
        result->address = cpu->TARGET;
    }
}
uint8_t ror_p2(CPU *cpu, uint8_t operand) {
    //printf("OPERANDD %X %X ", operand, cpu->P);
    uint8_t old_carry = cpu->P & 1;
    uint8_t new_carry;
    if (cpu->MODE == ACC) {
        new_carry = cpu->A & 0x01;
        cpu->A = (cpu->A >> 1) + (old_carry << 7);
        operand = cpu->A;
    } else {
        new_carry = operand & 0x01;
        //printf("OLD CARRRY %X %X", old_carry, cpu->P);
        operand = (operand >> 1) + (old_carry << 7);
    }
    
    cpu->P &= 0x7C;
    cpu->P |= new_carry;
    cpu->P = calc_NZ(cpu->P,operand);
    
    return operand;
}
// Shift Right
uint8_t lsr_p1(CPU *cpu,ACCESS *result) {
    if (cpu->MODE == ACC) {
        result->type = READ;
        result->address = cpu->PC;
    } else {
        result->type = READ;
        result->address = cpu->TARGET;
    }
}
uint8_t lsr_p2(CPU *cpu, uint8_t operand) {
    uint8_t new_carry;
    if (cpu->MODE == ACC) {
        new_carry = cpu->A & 0x01;
        cpu->A = cpu->A >> 1;
        operand = cpu->A;
    } else {
        new_carry = operand & 0x01;
        operand = operand >> 1;
    }
    
    cpu->P &= 0x7C;
    cpu->P += new_carry;
    cpu->P = calc_NZ(cpu->P,operand);
    
    return operand;
}

/*============================
    BRANCHING
 ===========================*/
// Branch on Carry Clear
uint8_t bcc_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t bcc_p2(CPU *cpu, uint8_t operand) { 
    if (!(cpu->P & 0x01)) {
        if (operand <= 127) {
            cpu->PC = cpu->PC + operand;
        } else {
            cpu->PC = cpu->PC - (255 - operand) - 1;
        }
    }
    
    return operand;
}
// Branch on Carry Set
uint8_t bcs_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t bcs_p2(CPU *cpu, uint8_t operand) { 
    if ((cpu->P & 0x01)) {
        if (operand <= 127) {
            cpu->PC = cpu->PC + operand;
        } else {
            cpu->PC = cpu->PC - (255 - operand) - 1;
        }
    }
    
    return operand;
}
// Branch on Not Equal (zero clear)
uint8_t bne_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t bne_p2(CPU *cpu, uint8_t operand) { 
    if (!(cpu->P & 0x02)) {
        if (operand <= 127) {
            cpu->PC = cpu->PC + operand;
        } else {
            cpu->PC = cpu->PC - (255 - operand) - 1;
        }
    }
    
    return operand;
}
// Branch on Equal (zero set)
uint8_t beq_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t beq_p2(CPU *cpu, uint8_t operand) { 
    if ((cpu->P & 0x02)) {
        if (operand <= 127) {
            cpu->PC = cpu->PC + operand;
        } else {
            cpu->PC = cpu->PC - (255 - operand) - 1;
        }
    }
    
    return operand;
}
// Branch on Overflow Clear
uint8_t bvc_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t bvc_p2(CPU *cpu, uint8_t operand) { 
    if (!(cpu->P & 0x40)) {
        if (operand <= 127) {
            cpu->PC = cpu->PC + operand;
        } else {
            cpu->PC = cpu->PC - (255 - operand) - 1;
        }
    }
    
    return operand;
}
// Branch on Overflow Set
uint8_t bvs_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t bvs_p2(CPU *cpu, uint8_t operand) { 
    if ((cpu->P & 0x40)) {
        if (operand <= 127) {
            cpu->PC = cpu->PC + operand;
        } else {
            cpu->PC = cpu->PC - (255 - operand) - 1;
        }
    }
    
    return operand;
}
// Branch on Plus (Negative Clear)
uint8_t bpl_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t bpl_p2(CPU *cpu, uint8_t operand) { 
    if (!(cpu->P & 0x80)) {
        if (operand <= 127) {
            cpu->PC = cpu->PC + operand;
        } else {
            cpu->PC = cpu->PC - (255 - operand) - 1;
        }
    }
    
    return operand;
}
// Branch on Negative (Negative Set)
uint8_t bmi_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t bmi_p2(CPU *cpu, uint8_t operand) { 
    if ((cpu->P & 0x80)) {
        if (operand <= 127) {
            cpu->PC = cpu->PC + operand;
        } else {
            cpu->PC = cpu->PC - (255 - operand);
        }
    }
    
    return operand;
}
// Branch Always
uint8_t bra_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t bra_p2(CPU *cpu, uint8_t operand) { 
    if (operand <= 127) {
            cpu->PC = cpu->PC + operand;
    } else {
        cpu->PC = cpu->PC - (255 - operand) - 1;
    }
    
    return operand;
}


/*============================
    STORING
 ===========================*/

// Store A
uint8_t sta_p1(CPU *cpu,ACCESS *result) {
    result->type = WRITE;
    result->address = cpu->TARGET;
    result->value = cpu->A;
}
uint8_t sta_p2(CPU *cpu, uint8_t operand) {    
    return operand;
}

// Store X
uint8_t stx_p1(CPU *cpu,ACCESS *result) {
    result->type = WRITE;
    result->address = cpu->TARGET;
    result->value = cpu->X;
}
uint8_t stx_p2(CPU *cpu, uint8_t operand) {    
    return operand;
}

// Store Y
uint8_t sty_p1(CPU *cpu,ACCESS *result) {
    result->type = WRITE;
    result->address = cpu->TARGET;
    result->value = cpu->Y;
}
uint8_t sty_p2(CPU *cpu, uint8_t operand) {    
    return operand;
}

// Store Zero
uint8_t stz_p1(CPU *cpu,ACCESS *result) {
    result->type = WRITE;
    result->address = cpu->TARGET;
    result->value = 0;
}
uint8_t stz_p2(CPU *cpu, uint8_t operand) {    
    return operand;
}
/*============================
    LOADING
 ===========================*/

// Load A
uint8_t lda_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t lda_p2(CPU *cpu, uint8_t operand) {
    cpu->A=operand;
    cpu->P=calc_NZ(cpu->P,operand);
    return operand;
}
// Load X
uint8_t ldx_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t ldx_p2(CPU *cpu, uint8_t operand) {
    cpu->X=operand;
    cpu->P=calc_NZ(cpu->P,operand);
    return operand;
}
// Load Y
uint8_t ldy_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->TARGET;
}
uint8_t ldy_p2(CPU *cpu, uint8_t operand) {
    cpu->Y=operand;
    cpu->P=calc_NZ(cpu->P,operand);
    return operand;
}
/*============================
    Register Transfer
 ===========================*/
//--
uint8_t tax_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->PC;
}
uint8_t tax_p2(CPU *cpu, uint8_t operand) {
    cpu->X = cpu->A;
    cpu->P=calc_NZ(cpu->P,cpu->X);
    return operand;
}
//
uint8_t txa_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->PC;
}
uint8_t txa_p2(CPU *cpu, uint8_t operand) {
    cpu->A = cpu->X;
    cpu->P=calc_NZ(cpu->P,cpu->A);
    return operand;
}
//--
uint8_t tay_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->PC;
}
uint8_t tay_p2(CPU *cpu, uint8_t operand) {
    cpu->Y = cpu->A;
    cpu->P=calc_NZ(cpu->P,cpu->Y);
    return operand;
}
//
uint8_t tya_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->PC;
}
uint8_t tya_p2(CPU *cpu, uint8_t operand) {
    cpu->A = cpu->Y;
    cpu->P=calc_NZ(cpu->P,cpu->A);
    return operand;
}
//--
uint8_t txs_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->PC;
}
uint8_t txs_p2(CPU *cpu, uint8_t operand) {
    cpu->S = cpu->X;
    return operand;
}
//
uint8_t tsx_p1(CPU *cpu,ACCESS *result) {
    result->type = READ;
    result->address = cpu->PC;
}
uint8_t tsx_p2(CPU *cpu, uint8_t operand) {
    cpu->X = cpu->S;
    cpu->P=calc_NZ(cpu->P,cpu->X);
    return operand;
}

/*============================
    FLAGS
 ===========================*/
 // Carry
uint8_t sec_p1(CPU *cpu,ACCESS *result) {
    result->type = READ; result->address = cpu->PC;
}
uint8_t sec_p2(CPU *cpu, uint8_t operand) {
    cpu->P = cpu->P | 0x01; return operand;
}
uint8_t clc_p1(CPU *cpu,ACCESS *result) {
    result->type = READ; result->address = cpu->PC;
}
uint8_t clc_p2(CPU *cpu, uint8_t operand) {
    cpu->P = cpu->P & 0xFE; return operand;
}
 // Decimal
uint8_t sed_p1(CPU *cpu,ACCESS *result) {
    result->type = READ; result->address = cpu->PC;
}
uint8_t sed_p2(CPU *cpu, uint8_t operand) {
    cpu->P = cpu->P | 0x08; return operand;
}
uint8_t cld_p1(CPU *cpu,ACCESS *result) {
    result->type = READ; result->address = cpu->PC;
}
uint8_t cld_p2(CPU *cpu, uint8_t operand) {
    cpu->P = cpu->P & 0xF7; return operand;
}
 // Interrupt
uint8_t sei_p1(CPU *cpu,ACCESS *result) {
    result->type = READ; result->address = cpu->PC;
}
uint8_t sei_p2(CPU *cpu, uint8_t operand) {
    cpu->P = cpu->P | 0x04; return operand;
}
uint8_t cli_p1(CPU *cpu,ACCESS *result) {
    result->type = READ; result->address = cpu->PC;
}
uint8_t cli_p2(CPU *cpu, uint8_t operand) {
    cpu->P = cpu->P & 0xFB; return operand;
}
 // Overflow
uint8_t clv_p1(CPU *cpu,ACCESS *result) {
    result->type = READ; result->address = cpu->PC;
}
uint8_t clv_p2(CPU *cpu, uint8_t operand) {
    cpu->P = cpu->P & 0xBf; return operand;
}

/*============================
    REGISTER INC AND DEC
 ===========================*/
  // A
uint8_t ina_p1(CPU *cpu,ACCESS *result) {
    result->type = READ; result->address = cpu->PC;
}
uint8_t ina_p2(CPU *cpu, uint8_t operand) {
    cpu->A = cpu->A + 1;
    cpu->P = calc_NZ(cpu->P,cpu->X);
}
uint8_t dea_p1(CPU *cpu,ACCESS *result) {
    result->type = READ; result->address = cpu->PC;
}
uint8_t dea_p2(CPU *cpu, uint8_t operand) {
    cpu->A = cpu->A - 1;
    cpu->P = calc_NZ(cpu->P,cpu->A);
}
  // X
uint8_t inx_p1(CPU *cpu,ACCESS *result) {
    result->type = READ; result->address = cpu->PC;
}
uint8_t inx_p2(CPU *cpu, uint8_t operand) {
    cpu->X = cpu->X + 1;
    cpu->P = calc_NZ(cpu->P,cpu->X);
}
uint8_t dex_p1(CPU *cpu,ACCESS *result) {
    result->type = READ; result->address = cpu->PC;
}
uint8_t dex_p2(CPU *cpu, uint8_t operand) {
    cpu->X = cpu->X - 1;
    cpu->P = calc_NZ(cpu->P,cpu->X);
}
 // Y
uint8_t iny_p1(CPU *cpu,ACCESS *result) {
    result->type = READ; result->address = cpu->PC;
}
uint8_t iny_p2(CPU *cpu, uint8_t operand) {
    cpu->Y = cpu->Y + 1;
    cpu->P = calc_NZ(cpu->P,cpu->Y);
}
uint8_t dey_p1(CPU *cpu,ACCESS *result) {
    result->type = READ; result->address = cpu->PC;
}
uint8_t dey_p2(CPU *cpu, uint8_t operand) {
    cpu->Y = cpu->Y - 1;
    cpu->P = calc_NZ(cpu->P,cpu->Y);
}
 // MEMORY
uint8_t inc_p1(CPU *cpu,ACCESS *result) {
    result->type = READ; result->address = cpu->TARGET;
}
uint8_t inc_p2(CPU *cpu, uint8_t operand) {
    operand = operand + 1;
    cpu->P = calc_NZ(cpu->P,operand);
    return operand;
}
uint8_t dec_p1(CPU *cpu,ACCESS *result) {
    result->type = READ; result->address = cpu->TARGET;
}
uint8_t dec_p2(CPU *cpu, uint8_t operand) {
    operand = operand - 1;
    cpu->P = calc_NZ(cpu->P,operand);
    return operand;
}

/*============================
    STACK
 ===========================*/
 // ACCUMULATOR
uint8_t pha_p1(CPU *cpu,ACCESS *result) { 
    result->type = WRITE; result->address = cpu->S + 0x100;
    result->value = cpu->A;
}
uint8_t pha_p2(CPU *cpu, uint8_t operand) { return operand; }

uint8_t pla_p1(CPU *cpu,ACCESS *result) { 
    result->type = READ; result->address = cpu->S + 0x100;
}
uint8_t pla_p2(CPU *cpu, uint8_t operand) { 
    cpu->A = operand; 
    cpu->P = calc_NZ(cpu->P,operand);
}
 // X INDEX
uint8_t phx_p1(CPU *cpu,ACCESS *result) { 
    result->type = WRITE; result->address = cpu->S + 0x100;
    result->value = cpu->X;
}
uint8_t phx_p2(CPU *cpu, uint8_t operand) { return operand; }

uint8_t plx_p1(CPU *cpu,ACCESS *result) { 
    result->type = READ; result->address = cpu->S + 0x100;
}
uint8_t plx_p2(CPU *cpu, uint8_t operand) { cpu->X = operand; cpu->P = calc_NZ(cpu->P,operand);}
 // Y INDEX
uint8_t phy_p1(CPU *cpu,ACCESS *result) { 
    result->type = WRITE; result->address = cpu->S + 0x100;
    result->value = cpu->Y;
}
uint8_t phy_p2(CPU *cpu, uint8_t operand) { return operand; }

uint8_t ply_p1(CPU *cpu,ACCESS *result) { 
    result->type = READ; result->address = cpu->S + 0x100;
}
uint8_t ply_p2(CPU *cpu, uint8_t operand) { cpu->Y = operand; cpu->P = calc_NZ(cpu->P,operand);}
 // P FLAGS
uint8_t php_p1(CPU *cpu,ACCESS *result) { 
    result->type = WRITE; result->address = cpu->S + 0x100;
    result->value = cpu->P | 0x10;
}
uint8_t php_p2(CPU *cpu, uint8_t operand) { return operand; }

uint8_t plp_p1(CPU *cpu,ACCESS *result) { 
    result->type = READ; result->address = cpu->S + 0x100;
}
uint8_t plp_p2(CPU *cpu, uint8_t operand) { cpu->P = operand;}


// Uninplemented

uint8_t bbr_p1(CPU *cpu,ACCESS *result) { }
uint8_t bbr_p2(CPU *cpu, uint8_t operand) { }
uint8_t bbs_p1(CPU *cpu,ACCESS *result) { }
uint8_t bbs_p2(CPU *cpu, uint8_t operand) { }
uint8_t brk_p1(CPU *cpu,ACCESS *result) { }
uint8_t brk_p2(CPU *cpu, uint8_t operand) { }

uint8_t jmp_p1(CPU *cpu,ACCESS *result) { 
    result->type = READ;
}
uint8_t jmp_p2(CPU *cpu, uint8_t operand) { 
    cpu->PC = cpu->TARGET - 1;
}

uint8_t jsr_p1(CPU *cpu,ACCESS *result) { }
uint8_t jsr_p2(CPU *cpu, uint8_t operand) { }

uint8_t rmb_p1(CPU *cpu,ACCESS *result) { }
uint8_t rmb_p2(CPU *cpu, uint8_t operand) { }

uint8_t rti_p1(CPU *cpu,ACCESS *result) { }
uint8_t rti_p2(CPU *cpu, uint8_t operand) { }
uint8_t rts_p1(CPU *cpu,ACCESS *result) { }
uint8_t rts_p2(CPU *cpu, uint8_t operand) { }

uint8_t smb_p1(CPU *cpu,ACCESS *result) { }
uint8_t smb_p2(CPU *cpu, uint8_t operand) { }

uint8_t trb_p1(CPU *cpu,ACCESS *result) { }
uint8_t trb_p2(CPU *cpu, uint8_t operand) { }
uint8_t tsb_p1(CPU *cpu,ACCESS *result) { }
uint8_t tsb_p2(CPU *cpu, uint8_t operand) { }

uint8_t wai_p1(CPU *cpu,ACCESS *result) { }
uint8_t wai_p2(CPU *cpu, uint8_t operand) {
    if (cpu->IRQ || cpu->NMI || cpu->RESET) {
        cpu_opend(cpu);
    }
}
uint8_t stp_p1(CPU *cpu,ACCESS *result) { }
uint8_t stp_p2(CPU *cpu, uint8_t operand) { 
    if (cpu->RESET) {
        cpu_opend(cpu);
    }
}

// MODES
// HLD mode
uint8_t mode_hold_p1(CPU *cpu,ACCESS *result) {
    cpu->C = 1;
    result->type = READ;
    result->address = cpu->PC;
    phase_1[op_codes[cpu->I]](cpu, result);
}
uint8_t mode_hold_p2(CPU *cpu, uint8_t operand) {
    phase_2[op_codes[cpu->I]](cpu, operand);
}

// Implied -> 2 cycles, no operand
uint8_t mode_imp_p1(CPU *cpu,ACCESS *result) { 
    phase_1[op_codes[cpu->I]](cpu, result);
}
uint8_t mode_imp_p2(CPU *cpu, uint8_t operand) {
    phase_2[op_codes[cpu->I]](cpu, operand);
    cpu_opend(cpu);
}
// Imd -> 2 cycles, with one operand
uint8_t mode_imd_p1(CPU *cpu,ACCESS *result) { 
    cpu->PC += 1;
    cpu->TARGET = cpu->PC;
    phase_1[op_codes[cpu->I]](cpu, result);
}
uint8_t mode_imd_p2(CPU *cpu, uint8_t operand) {
    phase_2[op_codes[cpu->I]](cpu, operand);
    cpu_opend(cpu);
}
// Abs -> 4 cycles, 16-bit address + operation
uint8_t mode_abs_p1(CPU *cpu,ACCESS *result) {
    switch (cpu->C) {
        case 1:
        case 2:
            cpu->PC += 1;
            result->type = READ;
            result->address = cpu->PC;
            break;
        case 3:
            cpu->C += 1;
        case 4:
            phase_1[op_codes[cpu->I]](cpu, result); break;
        case 5: break;
        case 6:
            result->value = cpu->DL;
            result->type = WRITE;
            result->address = cpu->TARGET;
    }
}
uint8_t mode_abs_p2(CPU *cpu, uint8_t operand) {
    uint8_t extra = 0;
    if (cpu->MODE == AX) extra = cpu->X;
    if (cpu->MODE == AY) extra = cpu->Y;
        
    switch (cpu->C) {
        case 1:
            cpu->TARGET = 0 + operand; break;
        case 2:
            cpu->TARGET = cpu->TARGET + (operand << 8) + extra;
            cpu->PAGEFLIP = (cpu->TARGET & 0x00FF) + extra > 0xFF;
            break;
        case 3:
            break;
        case 4:
            cpu->DL = phase_2[op_codes[cpu->I]](cpu, operand);
            if (!cpu->RMW) {
                cpu_opend(cpu);
            }
            
            break;
        case 5: break;
        case 6:
            cpu_opend(cpu);
            break;
    }
}

uint8_t mode_jsr_p1(CPU *cpu,ACCESS *result) {
    switch (cpu->C) {
        case 1:
        case 2:
            cpu->PC += 1;
            result->type = READ;
            result->address = cpu->PC;
            break;
        case 3:
        case 4:
            if (cpu->C == 4) result->value = cpu->PC & 0x00FF;
            else result->value = (cpu->PC & 0xFF00) >> 8;
            
            result->type = WRITE;
            result->address = 0x100 + cpu->S;
            cpu->S -= 1;
            break;
        case 5:
            result->type = READ;
            result->address = cpu->TARGET;
    }
}
uint8_t mode_jsr_p2(CPU *cpu, uint8_t operand) {
    uint8_t extra = 0;

    switch (cpu->C) {
        case 1:
            cpu->TARGET = 0 + operand; break;
        case 2:
            cpu->TARGET = cpu->TARGET + (operand << 8) + extra;
            cpu->PAGEFLIP = (cpu->TARGET & 0x00FF) + extra > 0xFF;
            break;
        case 3:
        case 4:
            break;
        case 5:
            cpu->PC = cpu->TARGET - 1;
            cpu_opend(cpu);
            break;
    }
}

uint8_t mode_rts_p1(CPU *cpu,ACCESS *result) { 
    switch (cpu->C) {
        case 1:
        case 2:
            cpu->S += 1;
            result->type = READ;
            result->address = 0x100 + cpu->S;
            break;
        case 3:
            break;

    }
}
uint8_t mode_rts_p2(CPU *cpu, uint8_t operand) {
    switch (cpu->C) {
        case 1:
            cpu->TARGET = operand; break;
        case 2:
            cpu->TARGET += operand << 8; break;
        case 3:
            cpu->PC = cpu->TARGET;
            cpu_opend(cpu);
            break;

    }
}

uint8_t mode_rti_p1(CPU *cpu,ACCESS *result) { 
    switch (cpu->C) {
        case 1:
        case 2:
        case 3:
            cpu->S += 1;
            result->type = READ;
            result->address = 0x100 + cpu->S;
            break;
        case 4:
            break;

    }
}
uint8_t mode_rti_p2(CPU *cpu, uint8_t operand) {
    switch (cpu->C) {
        case 1:
            cpu->P = operand;
        case 2:
            cpu->TARGET = operand; break;
        case 3:
            cpu->TARGET += operand << 8; break;
        case 4:
            cpu->PC = cpu->TARGET-1;
            cpu_opend(cpu);
            break;

    }
}

// ABS Indirect
uint8_t mode_abs_ind_p1(CPU *cpu,ACCESS *result) {
    uint8_t extra = 0;
    if (cpu->MODE == IAX) {
        extra = cpu->X;
    }
    switch (cpu->C) {
        case 1:
        case 2:
            cpu->PC += 1;
            result->type = READ;
            result->address = cpu->PC;
            break;
        case 3:
        case 4:
            result->type = READ;
            result->address = cpu->TARGET;
            cpu->TARGET = (cpu->TARGET & 0xFF00) + ((cpu->TARGET+1) & 0x00FF);
            break;
        case 5:
            result->type = READ;
            result->address = cpu->DL;
            break;
        default:
            phase_1[op_codes[cpu->I]](cpu, result);
    }
}
uint8_t mode_abs_ind_p2(CPU *cpu, uint8_t operand) {
    
    switch (cpu->C) {
        case 1:
            cpu->TARGET = 0 + operand; break;
        case 2:
            cpu->TARGET = cpu->TARGET + (operand << 8); break;
        case 3:
            cpu->DL = 0 + operand; break;
        case 4:
            cpu->DL = cpu->DL + (operand << 8); break;
        case 5:
            cpu->TARGET = cpu->DL;
            cpu->DL = operand;
            break;
        default:
            phase_2[op_codes[cpu->I]](cpu, operand);
            cpu_opend(cpu);
    }
}

// Zp -> 3 cycles, 8-bit address + operation
uint8_t mode_zp_p1(CPU *cpu,ACCESS *result) { 
    switch (cpu->C) {
        case 1:
            cpu->PC += 1;
            result->type = READ;
            result->address = cpu->PC;
            break;
        case 2:
            phase_1[op_codes[cpu->I]](cpu, result); break;
        case 3:
            result->type = WRITE;
            result->address = cpu->TARGET;
            result->value = cpu->DL;
    }
}
uint8_t mode_zp_p2(CPU *cpu, uint8_t operand) {
    uint8_t extra = 0;
    if (cpu->MODE == ZX) extra = cpu->X;
    if (cpu->MODE == ZY) extra = cpu->Y;
    
    switch (cpu->C) {
        case 1:
            cpu->TARGET = (operand + extra) & 0xFF; break;
        case 2:
            cpu->DL = phase_2[op_codes[cpu->I]](cpu, operand);
            if (!cpu->RMW) cpu_opend(cpu);
            break;
        case 3:
            cpu_opend(cpu);
    }
}

// ZP Indirect
uint8_t mode_zp_ind_p1(CPU *cpu,ACCESS *result) {
    int8_t extra = 0;
    if (cpu->MODE == IZX) {
        extra = cpu->X;
    }
    
    switch (cpu->C) {
        case 1:
            cpu->PC += 1;
            result->type = READ;
            result->address = cpu->PC;
            break;
        case 2:
        case 3:
            result->type = READ;
            result->address = (cpu->TARGET + extra) & 0xFF;
            cpu->TARGET = (cpu->TARGET + 1) & 0xFF;
            break;
        case 4:
            result->type = READ;
            result->address = cpu->DL;
            break;
        default:
            phase_1[op_codes[cpu->I]](cpu, result);
    }
}
uint8_t mode_zp_ind_p2(CPU *cpu, uint8_t operand) {
    uint8_t extra = 0;
    if (cpu->MODE == IZY) extra = cpu->Y;
        
    switch (cpu->C) {
        case 1:
            cpu->TARGET = 0 + operand; break;
        case 2:
            cpu->DL = 0 + operand; break;
        case 3:
            cpu->DL = cpu->DL + (operand << 8); break;
        case 4:
            cpu->TARGET = cpu->DL + extra;
            cpu->DL = operand;
            break;
        default:
            phase_2[op_codes[cpu->I]](cpu, operand);
            cpu_opend(cpu);
    }
}
// Stack
uint8_t mode_push_p1(CPU *cpu,ACCESS *result) { 
    switch (cpu->C) {
        case 1:
            phase_1[op_codes[cpu->I]](cpu, result);
            cpu->S -= 1;
            break;
    }
}
uint8_t mode_push_p2(CPU *cpu, uint8_t operand) {
    switch (cpu->C) {
        case 1:
            phase_2[op_codes[cpu->I]](cpu, operand);
            cpu_opend(cpu);
            break;
    }
}
uint8_t mode_pull_p1(CPU *cpu,ACCESS *result) { 
    switch (cpu->C) {
        case 1:
            cpu->S += 1;
            phase_1[op_codes[cpu->I]](cpu, result);
            break;
    }
}
uint8_t mode_pull_p2(CPU *cpu, uint8_t operand) {
    switch (cpu->C) {
        case 1:
            phase_2[op_codes[cpu->I]](cpu, operand);
            cpu_opend(cpu);
        break;
    }
}

uint8_t mode_interrupt_p1(CPU *cpu,ACCESS *result) {
    cpu->C += 1;
    switch (cpu->C) {
        case 1:
            result->type = READ;
            result->address= cpu->PC; break;
        case 2:
            result->type = WRITE;
            result->address = 0x0100 + cpu->S;
            result->value = cpu->PC >> 8;

            cpu->S -= 1; break;
        case 3:
            result->type = WRITE;
            result->address = 0x0100 + cpu->S;
            result->value = cpu->PC & 0x00FF;

            cpu->S -= 1; break;
        case 4:
            result->type = WRITE;
            result->address = 0x0100 + cpu->S;
            result->value = cpu->S;

            cpu->S -= 1; break;
        case 5:
            result->type = READ;
            result->address = 0xFFF0; break;
        case 6:
            result->type = READ;
            result->address = 0xFFF0; break;
        default:
            break;
    }
}
uint8_t mode_interrupt_p2(CPU *cpu, uint8_t operand) {
    switch (cpu->C) {
        case 5:
            cpu->TARGET = operand; break;
        case 6:
            cpu->TARGET += operand << 8;
            cpu->PC = cpu->TARGET;
            cpu_opend(cpu); break;
        default:
            break;
    }
}

int cpu_tick1(CPU *cpu,ACCESS *result) {
    cpu->P |= 0x20;
    cpu->P &= 0xEF;
    
    result->type = READ;
    result->address = cpu->PC;
    
    if ( ((cpu->IRQ && !(cpu->P & 0x04)) || cpu->NMI || cpu->RESET) && cpu->C == 0) {
        cpu->IN_INTERRUPT = 1;
    }
    
    if (cpu->RESET || cpu->IN_INTERRUPT) {
        if (cpu->RESET) {
            // RESET
            mode_interrupt_p1(cpu, result);
            switch (cpu->C) {
                case 5:
                    result->address= 0xFFFC;
                    break;
                
                case 6:
                    result->address= 0xFFFD;
                    break;
                default:
                    break;
            }
        } else if (cpu->NMI) {
            // Non Maskable Interrupt
            mode_interrupt_p1(cpu, result);
            switch (cpu->C) {
                case 5:
                    result->address= 0xFFFA;
                    break;
                
                case 6:
                    result->address= 0xFFFB;
                    break;
                default:
                    break;
            }
        } else if (cpu->IRQ) {
            // Interrupt Request
           mode_interrupt_p1(cpu, result);
           switch (cpu->C) {
                case 5:
                    result->address= 0xFFFE;
                    break;
                case 6:
                    result->address= 0xFFFF;
                    break;
                default:
                    break;
            }
        }
    } else {
        // NORMAL OP 
        
        switch (cpu->C) {
            case 0:
                break;
            default:
                add_mode_1[cpu->MODE](cpu, result);
        }
        
    }
    return 0;
}
int cpu_tick2(CPU *cpu, uint8_t operand) {
    if (cpu->IN_INTERRUPT) {
        if (cpu->RESET) {
            // Reset
            mode_interrupt_p2(cpu, operand);
            switch (cpu->C) {
                case 0:
                    cpu->RESET = 0;
                    cpu->IN_INTERRUPT = 0;
                    break;
                default:
                    cpu->P = (cpu->P | 0x34) & 0xf7;
                break;
            }
        } else if (cpu->NMI) {
            // Non-Maskable Interrupt
            mode_interrupt_p2(cpu, operand);
            switch (cpu->C) {
                case 0:
                    cpu->NMI = 0;
                    cpu->IN_INTERRUPT = 0;
                    break;
                default:
                    break;
            }
        } else if (cpu->IRQ) {
            // Interrupt Request
            mode_interrupt_p2(cpu, operand); 
            switch (cpu->C) {
                case 0:
                    cpu->IRQ = 0;
                    cpu->IN_INTERRUPT = 0;
                    break;
                default:
                    break;
            }
        }
    } else {
        switch (cpu->C) {
            case 0:
                cpu->I = operand;
                cpu->MODE = op_modes[cpu->I];
                cpu->RMW  = op_rmw[cpu->I];
                cpu->C += 1;
                
                break;
            default:
                add_mode_2[cpu->MODE](cpu, operand);
                if (cpu->C != 0) {
                    cpu->C += 1;
                } else {
                    cpu->PC += 1;
                }
                break;
        }
    }
    
    return 0;
}

int w6502_setup() {

    phase_1[ADC] = adc_p1; phase_2[ADC] = adc_p2;
    phase_1[AND] = and_p1; phase_2[AND] = and_p2;
    phase_1[ASL] = asl_p1; phase_2[ASL] = asl_p2;
    phase_1[BBR] = bbr_p1; phase_2[BBR] = bbr_p2;
    phase_1[BBS] = bbs_p1; phase_2[BBS] = bbs_p2;
    phase_1[BCC] = bcc_p1; phase_2[BCC] = bcc_p2;
    phase_1[BCS] = bcs_p1; phase_2[BCS] = bcs_p2;
    phase_1[BEQ] = beq_p1; phase_2[BEQ] = beq_p2;
    phase_1[BIT] = bit_p1; phase_2[BIT] = bit_p2;
    phase_1[BMI] = bmi_p1; phase_2[BMI] = bmi_p2;
    
    phase_1[BNE] = bne_p1; phase_2[BNE] = bne_p2;
    phase_1[BPL] = bpl_p1; phase_2[BPL] = bpl_p2;
    phase_1[BRA] = bra_p1; phase_2[BRA] = bra_p2;
    phase_1[BRK] = brk_p1; phase_2[BRK] = brk_p2;
    phase_1[BVC] = bvc_p1; phase_2[BVC] = bvc_p2;
    phase_1[BVS] = bvs_p1; phase_2[BVS] = bvs_p2;
    phase_1[CLC] = clc_p1; phase_2[CLC] = clc_p2;
    phase_1[CLD] = cld_p1; phase_2[CLD] = cld_p2;
    phase_1[CLI] = cli_p1; phase_2[CLI] = cli_p2;
    phase_1[CLV] = clv_p1; phase_2[CLV] = clv_p2;
    
    phase_1[CMP] = cmp_p1; phase_2[CMP] = cmp_p2;
    phase_1[CPX] = cpx_p1; phase_2[CPX] = cpx_p2;
    phase_1[CPY] = cpy_p1; phase_2[CPY] = cpy_p2;
    phase_1[DEC] = dec_p1; phase_2[DEC] = dec_p2;
    phase_1[DEX] = dex_p1; phase_2[DEX] = dex_p2;
    phase_1[DEY] = dey_p1; phase_2[DEY] = dey_p2;
    phase_1[EOR] = eor_p1; phase_2[EOR] = eor_p2;
    phase_1[INC] = inc_p1; phase_2[INC] = inc_p2;
    phase_1[INX] = inx_p1; phase_2[INX] = inx_p2;
    phase_1[INY] = iny_p1; phase_2[INY] = iny_p2;
    
    phase_1[JMP] = jmp_p1; phase_2[JMP] = jmp_p2;
    phase_1[JSR] = jsr_p1; phase_2[JSR] = jsr_p2;
    phase_1[LDA] = lda_p1; phase_2[LDA] = lda_p2;
    phase_1[LDX] = ldx_p1; phase_2[LDX] = ldx_p2;
    phase_1[LDY] = ldy_p1; phase_2[LDY] = ldy_p2;
    phase_1[LSR] = lsr_p1; phase_2[LSR] = lsr_p2;
    phase_1[NOP] = nop_p1; phase_2[NOP] = nop_p2;
    phase_1[NO_OPCODE] = nop_p1; phase_2[NO_OPCODE] = nop_p2;
    phase_1[ORA] = ora_p1; phase_2[ORA] = ora_p2;
    phase_1[PHA] = pha_p1; phase_2[PHA] = pha_p2;
    phase_1[PHP] = php_p1; phase_2[PHP] = php_p2;
    
    phase_1[PHX] = phx_p1; phase_2[PHX] = phx_p2;
    phase_1[PHY] = phy_p1; phase_2[PHY] = phy_p2;
    phase_1[PLA] = pla_p1; phase_2[PLA] = pla_p2;
    phase_1[PLP] = plp_p1; phase_2[PLP] = plp_p2;
    phase_1[PLX] = plx_p1; phase_2[PLX] = plx_p2;
    phase_1[PLY] = ply_p1; phase_2[PLY] = ply_p2;
    phase_1[RMB] = rmb_p1; phase_2[RMB] = rmb_p2;
    phase_1[ROL] = rol_p1; phase_2[ROL] = rol_p2;
    phase_1[ROR] = ror_p1; phase_2[ROR] = ror_p2;
    phase_1[RTI] = rti_p1; phase_2[RTI] = rti_p2;
    
    phase_1[RTS] = rts_p1; phase_2[RTS] = rts_p2;
    phase_1[SBC] = sbc_p1; phase_2[SBC] = sbc_p2;
    phase_1[SEC] = sec_p1; phase_2[SEC] = sec_p2;
    phase_1[SED] = sed_p1; phase_2[SED] = sed_p2;
    phase_1[SEI] = sei_p1; phase_2[SEI] = sei_p2;
    phase_1[SMB] = smb_p1; phase_2[SMB] = smb_p2;
    phase_1[STA] = sta_p1; phase_2[STA] = sta_p2;
    phase_1[STP] = stp_p1; phase_2[STP] = stp_p2;
    phase_1[STX] = stx_p1; phase_2[STX] = stx_p2;
    phase_1[STY] = sty_p1; phase_2[STY] = sty_p2;
    
    phase_1[STZ] = stz_p1; phase_2[STZ] = stz_p2;
    phase_1[TAX] = tax_p1; phase_2[TAX] = tax_p2;
    phase_1[TAY] = tay_p1; phase_2[TAY] = tay_p2;
    phase_1[TRB] = trb_p1; phase_2[TRB] = trb_p2;
    phase_1[TSB] = tsb_p1; phase_2[TSB] = tsb_p2;
    phase_1[TSX] = tsx_p1; phase_2[TSX] = tsx_p2;
    phase_1[TXA] = txa_p1; phase_2[TXA] = txa_p2;
    phase_1[TXS] = txs_p1; phase_2[TXS] = txs_p2;
    phase_1[TYA] = tya_p1; phase_2[TYA] = tya_p2;
    phase_1[WAI] = wai_p1; phase_2[WAI] = wai_p2;
    
    phase_1[DEA] = dea_p1; phase_2[DEA] = dea_p2;
    phase_1[INA] = ina_p1; phase_2[INA] = ina_p2;
    
    add_mode_1[A]   = mode_abs_p1; add_mode_2[A] = mode_abs_p2;
    add_mode_1[IAX] = mode_abs_ind_p1; add_mode_2[IAX] = mode_abs_ind_p2;
    add_mode_1[AX]  = mode_abs_p1; add_mode_2[AX] = mode_abs_p2;
    add_mode_1[AY]  = mode_abs_p1; add_mode_2[AY] = mode_abs_p2;
    add_mode_1[IA]  = mode_abs_ind_p1; add_mode_2[IA] = mode_abs_ind_p2;
    add_mode_1[ACC] = mode_imp_p1; add_mode_2[ACC] = mode_imp_p2;
    add_mode_1[IMD] = mode_imd_p1; add_mode_2[IMD] = mode_imd_p2;
    add_mode_1[IMP] = mode_imp_p1; add_mode_2[IMP] = mode_imp_p2;
    add_mode_1[R]   = mode_imd_p1; add_mode_2[R] = mode_imd_p2;
    
    add_mode_1[SH]   = mode_push_p1; add_mode_2[SH] = mode_push_p2;
    add_mode_1[SL]   = mode_pull_p1; add_mode_2[SL] = mode_pull_p2;
    
    add_mode_1[Z]   = mode_zp_p1; add_mode_2[Z] = mode_zp_p2;
    add_mode_1[IZX] = mode_zp_ind_p1; add_mode_2[IZX] = mode_zp_ind_p2;
    add_mode_1[ZX]   = mode_zp_p1; add_mode_2[ZX] = mode_zp_p2;
    add_mode_1[ZY]   = mode_zp_p1; add_mode_2[ZY] = mode_zp_p2;
    add_mode_1[IZ] = mode_zp_ind_p1; add_mode_2[IZ] = mode_zp_ind_p2;
    add_mode_1[IZY] = mode_zp_ind_p1; add_mode_2[IZY] = mode_zp_ind_p2;
    
    add_mode_1[AJSR] = mode_jsr_p1; add_mode_2[AJSR] = mode_jsr_p2;
    add_mode_1[ARTS] = mode_rts_p1; add_mode_2[ARTS] = mode_rts_p2;
    add_mode_1[ARTI] = mode_rti_p1; add_mode_2[ARTI] = mode_rti_p2;

    add_mode_1[HLD] = mode_hold_p1; add_mode_2[HLD] = mode_hold_p2;
    add_mode_1[INT] = mode_interrupt_p1; add_mode_2[INT] = mode_interrupt_p2;

}
