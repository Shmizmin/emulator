#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>
#include <array>

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cassert>

#include "opcode.h"

static constexpr const auto is_debug = true;

struct Processor
{
    std::uint8_t A, B, C, D;
    
    union
    {
        std::uint8_t F;
        
        struct
        {
            std::uint8_t Zero : 1,
                        Carry : 1,
                      Greater : 1,
                         Halt : 1,
                              : 4;
        };
        
        //verifies strict 1-byte packing of the above structure
        static_assert(sizeof(decltype(Zero)) == sizeof(std::uint8_t));
    };
    
    std::uint16_t SP{ 0x7FFF }, IP{ 0x8000 };
    
    std::uint8_t insn;
    
    std::array<std::uint8_t, std::numeric_limits<std::uint16_t>::max() + 1> memory;
};

namespace
{
    std::uint8_t fetch8(Processor& cpu) noexcept
    {
        auto val = cpu.memory[cpu.IP];
        ++cpu.IP;
        return val;
    }

    std::uint16_t fetch16(Processor& cpu) noexcept
    {
        auto val1 = fetch8(cpu);
        auto val2 = fetch8(cpu);
        return std::uint16_t((val2 << 8) | val1);
    }

    void push8(Processor& cpu, std::uint8_t val) noexcept
    {
        cpu.memory[cpu.SP] = val;
        --cpu.SP;
    }

    void push16(Processor& cpu, std::uint16_t val) noexcept
    {
        push8(cpu, ((val & 0xFF00) >> 8));
        push8(cpu, ((val & 0x00FF) >> 0));
    }

    std::uint8_t pop8(Processor& cpu) noexcept
    {
        auto val = cpu.memory[cpu.SP];
        ++cpu.SP;
        return val;
    }

    std::uint16_t pop16(Processor& cpu) noexcept
    {
        auto val1 = pop8(cpu);
        auto val2 = pop8(cpu);
        return std::uint16_t((val2 << 8) | val1);
    }
    

	void reset(Processor& cpu) noexcept
	{
        cpu.A = 0x00;
        cpu.B = 0x00;
        cpu.C = 0x00;
        cpu.D = 0x00;
        cpu.F = 0x00;
        cpu.SP = 0x7FFF;
        cpu.IP = 0x8000;
        
	}
	
    std::uint8_t OnClock(Processor& cpu) noexcept
    {
        auto Acopy = cpu.A;
        auto Bcopy = cpu.B;
        auto Ccopy = cpu.C;
        auto Dcopy = cpu.D;
                
#define CheckGreater(r) if (cpu.r & 0b10000000) { cpu.Greater = 0; } else { cpu.Greater = 1; }
#define CheckZero(r)    if (cpu.r == 0)      { cpu.Zero = 1;}   else { cpu.Zero = 0; }
#define CheckCarry(r)   if (cpu.r < r##copy) { cpu.Carry = 1; } else { cpu.Carry = 0; }
#define INSN(x) case x: if constexpr (is_debug) std::puts(#x);
        
#define CheckAll(x) CheckZero(x); CheckCarry(x); CheckGreater(x);

        switch (auto insn = fetch8(cpu))
        {
            INSN(NOP) break;
			INSN(BRK) cpu.Halt = 1; std::cin.get(); break;
                
                
			INSN(ADC_A_B) cpu.A += cpu.B + cpu.Carry; CheckAll(A); break;
			INSN(ADC_A_C) cpu.A += cpu.C + cpu.Carry; CheckAll(A); break;
			INSN(ADC_A_D) cpu.A += cpu.D + cpu.Carry; CheckAll(A); break;

			INSN(ADC_B_A) cpu.B += cpu.A + cpu.Carry; CheckAll(B); break;
			INSN(ADC_B_C) cpu.B += cpu.C + cpu.Carry; CheckAll(B); break;
			INSN(ADC_B_D) cpu.B += cpu.D + cpu.Carry; CheckAll(B); break;

			INSN(ADC_C_A) cpu.C += cpu.A + cpu.Carry; CheckAll(C); break;
			INSN(ADC_C_B) cpu.C += cpu.B + cpu.Carry; CheckAll(C); break;
			INSN(ADC_C_D) cpu.C += cpu.D + cpu.Carry; CheckAll(C); break;

			INSN(ADC_D_A) cpu.D += cpu.A + cpu.Carry; CheckAll(D); break;
			INSN(ADC_D_B) cpu.D += cpu.B + cpu.Carry; CheckAll(D); break;
			INSN(ADC_D_C) cpu.D += cpu.C + cpu.Carry; CheckAll(D); break;

			INSN(ADC_A_IMM) cpu.A += fetch8(cpu); CheckAll(A); break;
			INSN(ADC_B_IMM) cpu.B += fetch8(cpu); CheckAll(B); break;
			INSN(ADC_C_IMM) cpu.C += fetch8(cpu); CheckAll(C); break;
			INSN(ADC_D_IMM) cpu.D += fetch8(cpu); CheckAll(D); break;
             
			INSN(ADC_A_MEM) cpu.A += cpu.memory[fetch16(cpu)]; CheckAll(A); break;
			INSN(ADC_B_MEM) cpu.B += cpu.memory[fetch16(cpu)]; CheckAll(B); break;
			INSN(ADC_C_MEM) cpu.C += cpu.memory[fetch16(cpu)]; CheckAll(C); break;
			INSN(ADC_D_MEM) cpu.D += cpu.memory[fetch16(cpu)]; CheckAll(D); break;
             
             

			INSN(SBB_A_B) cpu.A -= cpu.B - cpu.Carry; CheckAll(A); break;
			INSN(SBB_A_C) cpu.A -= cpu.C - cpu.Carry; CheckAll(A); break;
			INSN(SBB_A_D) cpu.A -= cpu.D - cpu.Carry; CheckAll(A); break;

			INSN(SBB_B_A) cpu.B -= cpu.A - cpu.Carry; CheckAll(B); break;
			INSN(SBB_B_C) cpu.B -= cpu.C - cpu.Carry; CheckAll(B); break;
			INSN(SBB_B_D) cpu.B -= cpu.D - cpu.Carry; CheckAll(B); break;
 
			INSN(SBB_C_A) cpu.C -= cpu.A - cpu.Carry; CheckAll(C); break;
			INSN(SBB_C_B) cpu.C -= cpu.B - cpu.Carry; CheckAll(C); break;
			INSN(SBB_C_D) cpu.C -= cpu.D - cpu.Carry; CheckAll(C); break;

			INSN(SBB_D_A) cpu.D -= cpu.A - cpu.Carry; CheckAll(D); break;
			INSN(SBB_D_B) cpu.D -= cpu.B - cpu.Carry; CheckAll(D); break;
			INSN(SBB_D_C) cpu.D -= cpu.C - cpu.Carry; CheckAll(D); break;

			INSN(SBB_A_IMM) cpu.A -= fetch8(cpu); CheckAll(A); break;
			INSN(SBB_B_IMM) cpu.B -= fetch8(cpu); CheckAll(B); break;
			INSN(SBB_C_IMM) cpu.C -= fetch8(cpu); CheckAll(C); break;
			INSN(SBB_D_IMM) cpu.D -= fetch8(cpu); CheckAll(D); break;
			
			INSN(SBB_A_MEM) cpu.A -= cpu.memory[fetch16(cpu)]; CheckAll(A); break;
			INSN(SBB_B_MEM) cpu.B -= cpu.memory[fetch16(cpu)]; CheckAll(B); break;
			INSN(SBB_C_MEM) cpu.C -= cpu.memory[fetch16(cpu)]; CheckAll(C); break;
			INSN(SBB_D_MEM) cpu.D -= cpu.memory[fetch16(cpu)]; CheckAll(D); break;
             
             
             
			INSN(AND_A_B) cpu.A &= cpu.B; CheckAll(A); break;
			INSN(AND_A_C) cpu.A &= cpu.C; CheckAll(A); break;
			INSN(AND_A_D) cpu.A &= cpu.D; CheckAll(A); break;

			INSN(AND_B_A) cpu.B &= cpu.A; CheckAll(B); break;
			INSN(AND_B_C) cpu.B &= cpu.C; CheckAll(B); break;
			INSN(AND_B_D) cpu.B &= cpu.D; CheckAll(B); break;

			INSN(AND_C_A) cpu.C &= cpu.A; CheckAll(C); break;
			INSN(AND_C_B) cpu.C &= cpu.B; CheckAll(C); break;
			INSN(AND_C_D) cpu.C &= cpu.D; CheckAll(C); break;

			INSN(AND_D_A) cpu.D &= cpu.A; CheckAll(D); break;
			INSN(AND_D_B) cpu.D &= cpu.B; CheckAll(D); break;
			INSN(AND_D_C) cpu.D &= cpu.C; CheckAll(D); break;
             
			INSN(AND_A_IMM) cpu.A &= fetch8(cpu); CheckAll(A); break;
			INSN(AND_B_IMM) cpu.B &= fetch8(cpu); CheckAll(B); break;
			INSN(AND_C_IMM) cpu.C &= fetch8(cpu); CheckAll(C); break;
			INSN(AND_D_IMM) cpu.D &= fetch8(cpu); CheckAll(D); break;
             
			INSN(AND_A_MEM) cpu.A &= cpu.memory[fetch16(cpu)]; CheckAll(A); break;
			INSN(AND_B_MEM) cpu.B &= cpu.memory[fetch16(cpu)]; CheckAll(B); break;
			INSN(AND_C_MEM) cpu.C &= cpu.memory[fetch16(cpu)]; CheckAll(C); break;
			INSN(AND_D_MEM) cpu.D &= cpu.memory[fetch16(cpu)]; CheckAll(D); break;
             

             
			INSN(LOR_A_B) cpu.A |= cpu.B; CheckAll(A); break;
			INSN(LOR_A_C) cpu.A |= cpu.C; CheckAll(A); break;
			INSN(LOR_A_D) cpu.A |= cpu.D; CheckAll(A); break;

			INSN(LOR_B_A) cpu.B |= cpu.A; CheckAll(B); break;
			INSN(LOR_B_C) cpu.B |= cpu.C; CheckAll(B); break;
			INSN(LOR_B_D) cpu.B |= cpu.D; CheckAll(B); break;

			INSN(LOR_C_A) cpu.C |= cpu.A; CheckAll(C); break;
			INSN(LOR_C_B) cpu.C |= cpu.B; CheckAll(C); break;
			INSN(LOR_C_D) cpu.C |= cpu.D; CheckAll(C); break;

			INSN(LOR_D_A) cpu.D |= cpu.A; CheckAll(D); break;
			INSN(LOR_D_B) cpu.D |= cpu.B; CheckAll(D); break;
			INSN(LOR_D_C) cpu.D |= cpu.C; CheckAll(D); break;

			INSN(LOR_A_IMM) cpu.A |= fetch8(cpu); CheckAll(A); break;
			INSN(LOR_B_IMM) cpu.B |= fetch8(cpu); CheckAll(B); break;
			INSN(LOR_C_IMM) cpu.C |= fetch8(cpu); CheckAll(C); break;
			INSN(LOR_D_IMM) cpu.D |= fetch8(cpu); CheckAll(D); break;
            
			INSN(LOR_A_MEM) cpu.A |= cpu.memory[fetch16(cpu)]; CheckAll(A); break;
			INSN(LOR_B_MEM) cpu.B |= cpu.memory[fetch16(cpu)]; CheckAll(B); break;
			INSN(LOR_C_MEM) cpu.C |= cpu.memory[fetch16(cpu)]; CheckAll(C); break;
			INSN(LOR_D_MEM) cpu.D |= cpu.memory[fetch16(cpu)]; CheckAll(D); break;
                
                
            
            INSN(NOT_A) cpu.A = ~cpu.A; CheckZero(A); break;
            INSN(NOT_B) cpu.B = ~cpu.B; CheckZero(B); break;
            INSN(NOT_C) cpu.C = ~cpu.C; CheckZero(C); break;
            INSN(NOT_D) cpu.D = ~cpu.D; CheckZero(D); break;

                 
			INSN(ROL_A_IMM) cpu.A <<= fetch8(cpu); CheckZero(A); break;
			INSN(ROL_B_IMM) cpu.B <<= fetch8(cpu); CheckZero(B); break;
			INSN(ROL_C_IMM) cpu.C <<= fetch8(cpu); CheckZero(C); break;
			INSN(ROL_D_IMM) cpu.D <<= fetch8(cpu); CheckZero(D); break;

			INSN(ROR_A_IMM) cpu.A >>= fetch8(cpu); CheckZero(A); break;
			INSN(ROR_B_IMM) cpu.B >>= fetch8(cpu); CheckZero(B); break;
			INSN(ROR_C_IMM) cpu.C >>= fetch8(cpu); CheckZero(C); break;
			INSN(ROR_D_IMM) cpu.D >>= fetch8(cpu); CheckZero(D); break;

                 
            INSN(LDB_A_IMM) cpu.A = fetch8(cpu); break;
            INSN(LDB_B_IMM) cpu.B = fetch8(cpu); break;
            INSN(LDB_C_IMM) cpu.C = fetch8(cpu); break;
            INSN(LDB_D_IMM) cpu.D = fetch8(cpu); break;

            INSN(LDB_A_MEM) cpu.A = cpu.memory[fetch16(cpu)]; break;
            INSN(LDB_B_MEM) cpu.B = cpu.memory[fetch16(cpu)]; break;
            INSN(LDB_C_MEM) cpu.C = cpu.memory[fetch16(cpu)]; break;
            INSN(LDB_D_MEM) cpu.D = cpu.memory[fetch16(cpu)]; break;

            
            INSN(STB_MEM_A) cpu.memory[fetch16(cpu)] = cpu.A; break;
            INSN(STB_MEM_B) cpu.memory[fetch16(cpu)] = cpu.B; break;
            INSN(STB_MEM_C) cpu.memory[fetch16(cpu)] = cpu.C; break;
            INSN(STB_MEM_D) cpu.memory[fetch16(cpu)] = cpu.D; break;
            INSN(STB_MEM_IMM) cpu.memory[fetch16(cpu)] = fetch8(cpu); break;
                
            INSN(MVB_A_B) cpu.A = cpu.B; break;
            INSN(MVB_A_C) cpu.A = cpu.C; break;
            INSN(MVB_A_D) cpu.A = cpu.D; break;
            INSN(MVB_B_A) cpu.B = cpu.A; break;
            INSN(MVB_B_C) cpu.B = cpu.C; break;
            INSN(MVB_B_D) cpu.B = cpu.D; break;
            INSN(MVB_C_A) cpu.C = cpu.A; break;
            INSN(MVB_C_B) cpu.C = cpu.B; break;
            INSN(MVB_C_D) cpu.C = cpu.D; break;
            INSN(MVB_D_A) cpu.D = cpu.A; break;
            INSN(MVB_D_B) cpu.D = cpu.B; break;
            INSN(MVB_D_C) cpu.D = cpu.C; break;
				
			INSN(MVB_A_F) cpu.A = cpu.F; break;
			INSN(MVB_B_F) cpu.B = cpu.F; break;
			INSN(MVB_C_F) cpu.C = cpu.F; break;
			INSN(MVB_D_F) cpu.D = cpu.F; break;
			INSN(MVB_F_A) cpu.F = cpu.A; break;
			INSN(MVB_F_B) cpu.F = cpu.B; break;
			INSN(MVB_F_C) cpu.F = cpu.C; break;
			INSN(MVB_F_D) cpu.F = cpu.D; break;

			INSN(JEZ_MEM) if (cpu.Zero)    { cpu.IP = fetch16(cpu); } else { static_cast<void>(fetch16(cpu)); } break;
			INSN(JGZ_MEM) if (cpu.Greater) { cpu.IP = fetch16(cpu); } else { static_cast<void>(fetch16(cpu)); } break;
			INSN(JCS_MEM) if (cpu.Carry)   { cpu.IP = fetch16(cpu); } else { static_cast<void>(fetch16(cpu)); } break;
				
            INSN(PUSH_A)   push8(cpu, cpu.A); break;
            INSN(PUSH_B)   push8(cpu, cpu.B); break;
            INSN(PUSH_C)   push8(cpu, cpu.C); break;
            INSN(PUSH_D)   push8(cpu, cpu.D); break;
            INSN(PUSH_F)   push8(cpu, cpu.F); break;
            INSN(PUSH_IP)  push16(cpu, cpu.IP); break;
            INSN(PUSH_IMM) push8(cpu, fetch8(cpu)); break;
            INSN(PUSH_MEM) push8(cpu, cpu.memory[fetch8(cpu)]); break;

            INSN(POP_A)  cpu.A = pop8(cpu); break;
            INSN(POP_B)  cpu.B = pop8(cpu); break;
            INSN(POP_C)  cpu.C = pop8(cpu); break;
            INSN(POP_D)  cpu.D = pop8(cpu); break;
            INSN(POP_F)  cpu.F = pop8(cpu); break;
            INSN(POP_IP) cpu.IP = pop16(cpu); break;
            INSN(POP_DISCARD) static_cast<void>(pop8(cpu)); break;
                
                
			
			INSN(DEREF_AB_A) push8(cpu, cpu.B); push8(cpu, cpu.A); cpu.A = cpu.memory[pop16(cpu)]; break;
			INSN(DEREF_CD_C) push8(cpu, cpu.D); push8(cpu, cpu.C); cpu.C = cpu.memory[pop16(cpu)]; break;
				
			default: std::fprintf(stderr, "[Error] Instruction $%X is unimplemented.\nExiting...\n", insn); std::exit(EXIT_FAILURE);
        }
        
#undef CheckZero
#undef CheckCarry
#undef CheckGreater
#undef CheckAll
#undef INSN
        
        return !cpu.Halt;
    }
}


int main(int argc, const char** argv)
{
    
    if (argc != 2)
    {
        std::fprintf(stderr, "[Error] Expected 2 command line arguments but got %i\nExiting...\n", argc);
        return EXIT_FAILURE;
    }
   
    const auto path = argv[1];
    const auto size = std::filesystem::file_size(path);
    auto file = std::ifstream(path, std::ios::in | std::ios::binary);
    
    if (!file.good())
    {
        std::fprintf(stderr, "[Error] Could not open file %s for reading\nExiting...\n", path);
        return EXIT_FAILURE;
    }
    
    std::vector<std::uint8_t> buffer(size, '\0');
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    
    Processor processor{ 0 };
	reset(processor);
	
	std::copy(buffer.begin(), buffer.end(), processor.memory.begin());
    
#define u(x) static_cast<unsigned>(x)
    
    while (OnClock(processor))
    {
        std::printf("A: %u\nB: %u\nC: %u\nD: %u\nF: %u\nSP: %u\nIP: %u\n",
            u(processor.A),
            u(processor.B),
            u(processor.C),
            u(processor.D),
            u(processor.F),
            u(processor.SP),
            u(processor.IP));
        
        std::getchar();
    }
    
    
    return EXIT_SUCCESS;
}
