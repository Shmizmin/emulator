//#include <fstream>
//#include <filesystem>
//#include <iostream>
//#include <vector>
//#include <array>

#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "opcode.h"

#define is_debug true

typedef struct
{
    uint8_t A, B, C, D;
    
    union
    {
        uint8_t F;
        
        struct
        {
            uint8_t    Zero : 1,
                      Carry : 1,
                    Greater : 1,
                       Halt : 1,
                            : 4;
        };
    };
    
    uint16_t SP, IP;
    
    uint8_t insn;
    
    uint8_t memory[USHRT_MAX + 1];
} Processor;



static uint8_t fetch8(Processor* cpu)
{
    uint8_t val = cpu->memory[cpu->IP];
    ++cpu->IP;
    return val;
}

static uint16_t fetch16(Processor* cpu)
{
    uint8_t val1 = fetch8(cpu);
    uint8_t val2 = fetch8(cpu);
    
    uint16_t val_out = 0;
    
    val_out |= (val2 << 8);
    val_out |= (val1 << 0);
    
    return val_out;
}

static void push8(Processor* cpu, uint8_t val)
{
    cpu->memory[cpu->SP] = val;
    --cpu->SP;
}

static void push16(Processor* cpu, uint16_t val)
{
    push8(cpu, ((val & 0xFF00) >> 8));
    push8(cpu, ((val & 0x00FF) >> 0));
}

static uint8_t pop8(Processor* cpu)
{
    uint8_t val = cpu->memory[cpu->SP];
    ++cpu->SP;
    return val;
}

static uint16_t pop16(Processor* cpu)
{
    uint8_t val1 = pop8(cpu);
    uint8_t val2 = pop8(cpu);
    
    uint16_t val_out = 0;
    val_out |= (val2 << 8);
    val_out |= (val1 << 0);
    
    return val_out;
}

static void reset(Processor* cpu)
{
    cpu->A = 0x00;
    cpu->B = 0x00;
    cpu->C = 0x00;
    cpu->D = 0x00;
    cpu->F = 0x00;
    
    cpu->SP = 0x7FFF;
    cpu->IP = 0x8000;
    
    for (int i = 0; i < (USHRT_MAX + 1); ++i)
    {
        cpu->memory[i] = 0x00;
    }
}


static uint8_t OnClock(Processor* cpu)
{
    uint8_t Acopy = cpu->A,
            Bcopy = cpu->B,
            Ccopy = cpu->C,
            Dcopy = cpu->D;
            
#define CheckGreater(r) if (cpu->r & 0b10000000) { cpu->Greater = 0; } else { cpu->Greater = 1; }
#define CheckZero(r)    if (cpu->r == 0)         { cpu->Zero = 1; }    else { cpu->Zero = 0; }
#define CheckCarry(r)   if (cpu->r < r##copy)    { cpu->Carry = 1; }   else { cpu->Carry = 0; }
#define CheckAll(x) CheckZero(x); CheckCarry(x); CheckGreater(x);
    
#ifdef is_debug
#define INSN(x) case x: fprintf(stderr, #x);
#else
#define INSN(x) case x:
#endif
    
    switch (cpu->insn = fetch8(cpu))
    {
        INSN(NOP) break;
		INSN(BRK) cpu->Halt = 1; getchar(); break;
            
#define ALU_INSN_O(x, y, z, w, e) INSN(z##_##x##_##e) cpu->x y w; CheckAll(x); break;
#define ENUM_ALU_O(x) \
        ALU_INSN_O(x, +=, ADC, fetch8(cpu) + cpu->Carry, IMM) \
        ALU_INSN_O(x, +=, ADC, cpu->memory[fetch16(cpu)], MEM) \
        ALU_INSN_O(x, -=, SBB, fetch8(cpu) - cpu->Carry, IMM) \
        ALU_INSN_O(x, -=, SBB, cpu->memory[fetch16(cpu)], MEM) \
        ALU_INSN_O(x, &=, AND, fetch8(cpu), IMM) \
        ALU_INSN_O(x, &=, AND, cpu->memory[fetch16(cpu)], MEM) \
        ALU_INSN_O(x, |=, LOR, fetch8(cpu), IMM) \
        ALU_INSN_O(x, |=, LOR, cpu->memory[fetch16(cpu)], MEM)
        ENUM_ALU_O(A)
        ENUM_ALU_O(B)
        ENUM_ALU_O(C)
        ENUM_ALU_O(D)
#undef ENUM_ALU_O
#undef ALU_INSN_O
            
#define ALU_INSN_R(x, y, z, w, e) INSN(z##_##x##_##y) cpu->x w cpu->y e; CheckAll(x); break;
#define ENUM_ALU_R(x, y) \
        ALU_INSN_R(x, y, ADC, +=, + cpu->Carry) \
        ALU_INSN_R(x, y, SBB, -=, - cpu->Carry) \
        ALU_INSN_R(x, y, AND, &=, ) \
        ALU_INSN_R(x, y, LOR, |=, )
        ENUM_ALU_R(A, B)
        ENUM_ALU_R(A, C)
        ENUM_ALU_R(A, D)
            
        ENUM_ALU_R(B, A)
        ENUM_ALU_R(B, C)
        ENUM_ALU_R(B, D)
            
        ENUM_ALU_R(C, A)
        ENUM_ALU_R(C, B)
        ENUM_ALU_R(C, D)
            
        ENUM_ALU_R(D, A)
        ENUM_ALU_R(D, B)
        ENUM_ALU_R(D, C)
#undef ENUM_ALU_R
#undef ALU_INSN_R
            
            
#define NOT_INSN(x) INSN(NOT_##x) cpu->x = ~cpu->x; CheckZero(x); break;
        NOT_INSN(A)
        NOT_INSN(B)
        NOT_INSN(C)
        NOT_INSN(D)
#undef NOT_INSN

            
#define ROX_INSN(x, y, z) INSN(RO##y##_##x##_IMM) cpu->x z fetch8(cpu); CheckZero(x); break;
#define ENUM_ROX(x) ROX_INSN(x, L, <<=); ROX_INSN(x, R, >>=);
        ENUM_ROX(A)
        ENUM_ROX(B)
        ENUM_ROX(C)
        ENUM_ROX(D)
#undef ENUM_ROX
#undef ROX_INSN
        

#define LDB_IMM_INSN(x) INSN(LDB_##x##_IMM) cpu->x = fetch8(cpu); break;
#define LDB_MEM_INSN(x) INSN(LDB_##x##_MEM) cpu->x = cpu->memory[fetch16(cpu)]; break;
#define ENUM_LDB(x) LDB_IMM_INSN(x); LDB_MEM_INSN(x);
        ENUM_LDB(A)
        ENUM_LDB(B)
        ENUM_LDB(C)
        ENUM_LDB(D)
#undef ENUM_LDB
#undef LDB_MEM_INSN
#undef LDB_IMM_INSN
        
#define STB_INSN(x) cpu->memory[fetch16(cpu)] = cpu->x; break;
        STB_INSN(A)
        STB_INSN(B)
        STB_INSN(C)
        STB_INSN(D)
#undef STB_INSN

        INSN(STB_MEM_IMM) cpu->memory[fetch16(cpu)] = fetch8(cpu); break;
            

#define MVB_INSN(x, y) INSN(MVB_##x##_##y) cpu->x = cpu->y; break;
        MVB_INSN(A, B)
        MVB_INSN(A, C)
        MVB_INSN(A, D)
        MVB_INSN(A, F)
            
        MVB_INSN(B, A)
        MVB_INSN(B, C)
        MVB_INSN(B, D)
        MVB_INSN(B, F)
            
        MVB_INSN(C, A)
        MVB_INSN(C, B)
        MVB_INSN(C, D)
        MVB_INSN(C, F)
            
        MVB_INSN(D, A)
        MVB_INSN(D, B)
        MVB_INSN(D, C)
        MVB_INSN(D, F)
            
        MVB_INSN(F, A)
        MVB_INSN(F, B)
        MVB_INSN(F, C)
        MVB_INSN(F, D)
#undef MVB_INSN
            
#define JMP_INSN(x, y) INSN(x##_MEM) if (cpu->y) { cpu->IP = fetch16(cpu); } else { (void)fetch16(cpu); } break;
        JMP_INSN(JEZ, Zero)
        JMP_INSN(JGZ, Greater)
        JMP_INSN(JCS, Carry)
#undef JMP_INSN
            
#define PUSH_INSN(x, y) INSN(PUSH_##x) push##y(cpu, cpu->x); break;
        PUSH_INSN(A, 8)
        PUSH_INSN(B, 8)
        PUSH_INSN(C, 8)
        PUSH_INSN(D, 8)
        PUSH_INSN(F, 8)
        PUSH_INSN(IP, 16)
#undef PUSH_INSN

        INSN(PUSH_IMM) push8(cpu, fetch8(cpu)); break;
        INSN(PUSH_MEM) push8(cpu, cpu->memory[fetch8(cpu)]); break;

            
#define POP_INSN(x, y) INSN(POP_##x) cpu->x = pop##y(cpu); break;
        POP_INSN(A, 8)
        POP_INSN(B, 8)
        POP_INSN(C, 8)
        POP_INSN(D, 8)
        POP_INSN(F, 8)
        POP_INSN(IP, 16)
#undef POP_INSN
            
        INSN(POP_DISCARD) (void)pop8(cpu); break;
            
		INSN(DEREF_AB_A) push8(cpu, cpu->B); push8(cpu, cpu->A); cpu->A = cpu->memory[pop16(cpu)]; break;
		INSN(DEREF_CD_C) push8(cpu, cpu->D); push8(cpu, cpu->C); cpu->C = cpu->memory[pop16(cpu)]; break;
			
		default: fprintf(stderr, "[Error] Illegal instruction $%X was encountered.\nExiting...\n", cpu->insn); exit(EXIT_FAILURE);
    }
    
#undef CheckZero
#undef CheckCarry
#undef CheckGreater
#undef CheckAll
#undef INSN
    
    return !cpu->Halt;
}


int main(int argc, const char** argv)
{
    
    if (argc != 2)
    {
        fprintf(stderr, "[Error] Usage: emulator (filepath)\n");
        return EXIT_FAILURE;
    }
   
    const char* path = argv[1];
    FILE* file = fopen(path, "rb");
    
    //validates good file handle
    if (!file)
    {
        fprintf(stderr, "[Error] Could not open file %s for reading\nExiting...\n", path);
        return EXIT_FAILURE;
    }
    
    //get file size
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    
    uint8_t* buffer = malloc(size);
    fread((void*)buffer, sizeof(uint8_t), (size / sizeof(uint8_t)), file);
    
    Processor processor;
    memset(&processor, 0, sizeof(Processor));
    reset(&processor);

    
    //fill cpu ROM with program
    assert(SHRT_MAX == 32767);
    memcpy(&(processor.memory[0x8000]), &(buffer[0x8000]), SHRT_MAX);
    
#define u(x) (unsigned)x
    while (OnClock(&processor))
    {
        printf("A: %u\nB: %u\nC: %u\nD: %u\nF: %u\nSP: %u\nIP: %u\n",
            u(processor.A),
            u(processor.B),
            u(processor.C),
            u(processor.D),
            u(processor.F),
            u(processor.SP),
            u(processor.IP));
        
        getchar();
    }
#undef u
    
    return EXIT_SUCCESS;
}
