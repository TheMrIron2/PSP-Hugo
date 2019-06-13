//  h6280.c - Execute CPU instructions
//

#include <stdio.h>
#include <stdlib.h>

#include "interupt.h"
#include "dis_runtime.h"
#include "pce.h"
#include "hard_pce.h"
#include "gfx.h"
#include "pce.h"
#include "utils.h"

static int V_HSYNC = 455;

void
h6280_set_overclock(int overclock)
{
  V_HSYNC = 455 - overclock * 4;
}


# ifdef CODOP_PROFILE
  int codop_profile[256];

void dump_codop_stats()
{
  int i;
  for (i = 0; i < 256; ++i) {
    fprintf(stdout, "[%x] = %d\n", i, codop_profile[i] );
  }
}

# endif
// flag-value table (for speed)

UChar flnz_list[256] = {
  FL_Z,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,       // 00-0F
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,          // 40-4F
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,          // 70-7F
  FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,  // 80-87
  FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,
  FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,  // 90-97
  FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,
  FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,  // A0-A7
  FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,
  FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,  // B0-B7
  FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,
  FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,  // C0-C7
  FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,
  FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,  // D0-D7
  FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,
  FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,  // E0-E7
  FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,
  FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,  // F0-F7
  FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N,FL_N
};

// Elementary operations:
// - defined here for clarity, brevity,
//   and reduced typographical errors

// This code ignores hardware-segment accesses; it should only be used
// to access immediate data (hardware segment does not run code):
//
//  as a function:

static inline UChar imm_operand(UInt16 addr) {
  register unsigned short int memreg = addr>>13;
  return( (UChar) (PageR[memreg][addr]));
}

// This is the more generalized access routine:
static inline UChar get_8bit_addr(UInt16 addr) {
  register unsigned short int memreg = addr>>13;

  if (PageR[memreg] == IOAREA)
    return(IO_read(addr));
  else
    return((UChar) (PageR[memreg][addr]));
}

static inline void put_8bit_addr(UInt16 addr, UChar byte) {
  register unsigned int memreg = addr>>13;

  if (PageW[memreg] == IOAREA) {
    IO_write(addr, byte);
  } else {
    PageW[memreg][addr] = byte;
  }
}

static inline UInt16 get_16bit_addr(UInt16 addr) {
  register unsigned int memreg = addr>>13;
  UInt16 ret_16bit = (UChar) PageR[memreg][addr];
  memreg = (++addr)>>13;
  ret_16bit += (UInt16) ((UChar) PageR[memreg][addr] << 8);

  return(ret_16bit);
}

//Addressing modes:

#define abs_operand(x)     get_8bit_addr(get_16bit_addr(x))
#define absx_operand(x)    get_8bit_addr(get_16bit_addr(x)+reg_x)
#define absy_operand(x)    get_8bit_addr(get_16bit_addr(x)+reg_y)
#define zp_operand(x)      get_8bit_zp(imm_operand(x))
#define zpx_operand(x)     get_8bit_zp(imm_operand(x)+reg_x)
#define zpy_operand(x)     get_8bit_zp(imm_operand(x)+reg_y)
#define zpind_operand(x)   get_8bit_addr(get_16bit_zp(imm_operand(x)))
#define zpindx_operand(x)  get_8bit_addr(get_16bit_zp(imm_operand(x)+reg_x))
#define zpindy_operand(x)  get_8bit_addr(get_16bit_zp(imm_operand(x))+reg_y)

// Elementary flag check (flags 'N' and 'Z'):

#define chk_flnz_8bit(x) reg_p = ((reg_p & (~(FL_N|FL_T|FL_Z))) | flnz_list[x]);

static inline UChar get_8bit_zp(UChar zp_addr) {
  return((UChar) *(zp_base + zp_addr) );
}

static inline UInt16 get_16bit_zp(UChar zp_addr) {
  UInt16 n = *(zp_base + zp_addr);
  n += (*(zp_base + (UChar)(zp_addr+1)) << 8);
  return(n);
}

static inline void put_8bit_zp(UChar zp_addr, UChar byte) {
  *(zp_base + zp_addr) = byte;
}

static inline void push_8bit(UChar byte) {
  *(sp_base + reg_s--) = byte;
}

static inline UChar pull_8bit(void) {
  return((UChar) *(sp_base + ++reg_s) );
}

static inline void push_16bit(UInt16 addr) {
  *(sp_base + reg_s--) = (UChar)(addr>>8);
  *(sp_base + reg_s--) = (UChar)(addr&0xFF);
  return;
}

static inline UInt16 pull_16bit(void) {
  UInt16 n = (UChar) *(sp_base + ++reg_s);
  n += (UInt16)(((UChar) *(sp_base + ++reg_s)) << 8);
  return(n);
}

// Execute a single instruction :

static void
Int6502 (UChar Type)
{
  UInt16 J = 0;

  if ((Type == INT_NMI) || (!(reg_p & FL_I)))
  {
      cycles += 7;
      push_16bit(reg_pc);
      push_8bit(reg_p);
      reg_p = (reg_p & ~FL_D);

      if (Type == INT_NMI)
	{
	  J = VEC_NMI;
	}
      else
	  {
          reg_p |=  FL_I;

	  switch (Type)
	    {

	    case INT_IRQ:
	      J = VEC_IRQ;
	      break;

	    case INT_IRQ2:
	      J = VEC_IRQ2;
	      break;

	    case INT_TIMER:
	      J = VEC_TIMER;
	      break;

	    }

	  }
    reg_pc = get_16bit_addr((UInt16)J);
  }
}

//! Log all needed info to guess what went wrong in the cpu
void dump_pce_core() {

  int i;

  fprintf(stderr, "Dumping PCE core\n");

  Log("PC = 0x%04x\n", reg_pc);
  Log("A = 0x%02x\n", reg_a);
  Log("X = 0x%02x\n", reg_x);
  Log("Y = 0x%02x\n", reg_y);
  Log("P = 0x%02x\n", reg_p);
  Log("S = 0x%02x\n", reg_s);

  for (i = 0; i < 8; i++)
    {
      Log("MMR[%d] = 0x%02x\n", i, mmr[i]);
    }

  for (i = 0x2000; i < 0xFFFF; i++)
    {

      if ((i & 0xF) == 0)
        {
          Log("%04X: ", i);
        }

      Log("%02x ", get_8bit_addr((UInt16)i));
      if ((i & 0xF) == 0xF)
        {
          Log("\n");
        }
      if ((i & 0x1FFF) == 0x1FFF)
        {
          Log("\n-------------------------------------------------------------\n");
        }
    }

}


// Execute instructions as a machine would, including all
// important (known) interrupts, hardware functions, and
// actual video display on the hardware
//
// Until the following happens:
// (1) An unknown instruction is to be executed
// (2) An unknown hardware access is performed
// (3) <ESC> key is hit
//
#include "h6280-func.c"

//# define V_HSYNC 455
//# define V_HSYNC (455 - HUGO.hugo_overclock)

void exe_go(void) 
{
__label__
lab_0x0, lab_0x1, lab_0x2, lab_0x3, lab_0x4, lab_0x5, lab_0x6, lab_0x7,
lab_0x8, lab_0x9, lab_0xa, lab_0xb, lab_0xc, lab_0xd, lab_0xe, lab_0xf,
lab_0x10, lab_0x11, lab_0x12, lab_0x13, lab_0x14, lab_0x15, lab_0x16, lab_0x17,
lab_0x18, lab_0x19, lab_0x1a, lab_0x1b, lab_0x1c, lab_0x1d, lab_0x1e, lab_0x1f,
lab_0x20, lab_0x21, lab_0x22, lab_0x23, lab_0x24, lab_0x25, lab_0x26, lab_0x27,
lab_0x28, lab_0x29, lab_0x2a, lab_0x2b, lab_0x2c, lab_0x2d, lab_0x2e, lab_0x2f,
lab_0x30, lab_0x31, lab_0x32, lab_0x33, lab_0x34, lab_0x35, lab_0x36, lab_0x37,
lab_0x38, lab_0x39, lab_0x3a, lab_0x3b, lab_0x3c, lab_0x3d, lab_0x3e, lab_0x3f,
lab_0x40, lab_0x41, lab_0x42, lab_0x43, lab_0x44, lab_0x45, lab_0x46, lab_0x47,
lab_0x48, lab_0x49, lab_0x4a, lab_0x4b, lab_0x4c, lab_0x4d, lab_0x4e, lab_0x4f,
lab_0x50, lab_0x51, lab_0x52, lab_0x53, lab_0x54, lab_0x55, lab_0x56, lab_0x57,
lab_0x58, lab_0x59, lab_0x5a, lab_0x5b, lab_0x5c, lab_0x5d, lab_0x5e, lab_0x5f,
lab_0x60, lab_0x61, lab_0x62, lab_0x63, lab_0x64, lab_0x65, lab_0x66, lab_0x67,
lab_0x68, lab_0x69, lab_0x6a, lab_0x6b, lab_0x6c, lab_0x6d, lab_0x6e, lab_0x6f,
lab_0x70, lab_0x71, lab_0x72, lab_0x73, lab_0x74, lab_0x75, lab_0x76, lab_0x77,
lab_0x78, lab_0x79, lab_0x7a, lab_0x7b, lab_0x7c, lab_0x7d, lab_0x7e, lab_0x7f,
lab_0x80, lab_0x81, lab_0x82, lab_0x83, lab_0x84, lab_0x85, lab_0x86, lab_0x87,
lab_0x88, lab_0x89, lab_0x8a, lab_0x8b, lab_0x8c, lab_0x8d, lab_0x8e, lab_0x8f,
lab_0x90, lab_0x91, lab_0x92, lab_0x93, lab_0x94, lab_0x95, lab_0x96, lab_0x97,
lab_0x98, lab_0x99, lab_0x9a, lab_0x9b, lab_0x9c, lab_0x9d, lab_0x9e, lab_0x9f,
lab_0xa0, lab_0xa1, lab_0xa2, lab_0xa3, lab_0xa4, lab_0xa5, lab_0xa6, lab_0xa7,
lab_0xa8, lab_0xa9, lab_0xaa, lab_0xab, lab_0xac, lab_0xad, lab_0xae, lab_0xaf,
lab_0xb0, lab_0xb1, lab_0xb2, lab_0xb3, lab_0xb4, lab_0xb5, lab_0xb6, lab_0xb7,
lab_0xb8, lab_0xb9, lab_0xba, lab_0xbb, lab_0xbc, lab_0xbd, lab_0xbe, lab_0xbf,
lab_0xc0, lab_0xc1, lab_0xc2, lab_0xc3, lab_0xc4, lab_0xc5, lab_0xc6, lab_0xc7,
lab_0xc8, lab_0xc9, lab_0xca, lab_0xcb, lab_0xcc, lab_0xcd, lab_0xce, lab_0xcf,
lab_0xd0, lab_0xd1, lab_0xd2, lab_0xd3, lab_0xd4, lab_0xd5, lab_0xd6, lab_0xd7,
lab_0xd8, lab_0xd9, lab_0xda, lab_0xdb, lab_0xdc, lab_0xdd, lab_0xde, lab_0xdf,
lab_0xe0, lab_0xe1, lab_0xe2, lab_0xe3, lab_0xe4, lab_0xe5, lab_0xe6, lab_0xe7,
lab_0xe8, lab_0xe9, lab_0xea, lab_0xeb, lab_0xec, lab_0xed, lab_0xee, lab_0xef,
lab_0xf0, lab_0xf1, lab_0xf2, lab_0xf3, lab_0xf4, lab_0xf5, lab_0xf6, lab_0xf7,
lab_0xf8, lab_0xf9, lab_0xfa, lab_0xfb, lab_0xfc, lab_0xfd, lab_0xfe, lab_0xff;

    static const void* const a_jump_table[256] =  {
&&lab_0x0, &&lab_0x1, &&lab_0x2, &&lab_0x3, &&lab_0x4, &&lab_0x5, &&lab_0x6,
&&lab_0x7, &&lab_0x8, &&lab_0x9, &&lab_0xa, &&lab_0xb, &&lab_0xc, &&lab_0xd,
&&lab_0xe, &&lab_0xf, &&lab_0x10, &&lab_0x11, &&lab_0x12, &&lab_0x13,
&&lab_0x14, &&lab_0x15, &&lab_0x16, &&lab_0x17, &&lab_0x18, &&lab_0x19,
&&lab_0x1a, &&lab_0x1b, &&lab_0x1c, &&lab_0x1d, &&lab_0x1e, &&lab_0x1f,
&&lab_0x20, &&lab_0x21, &&lab_0x22, &&lab_0x23, &&lab_0x24, &&lab_0x25,
&&lab_0x26, &&lab_0x27, &&lab_0x28, &&lab_0x29, &&lab_0x2a, &&lab_0x2b,
&&lab_0x2c, &&lab_0x2d, &&lab_0x2e, &&lab_0x2f, &&lab_0x30, &&lab_0x31,
&&lab_0x32, &&lab_0x33, &&lab_0x34, &&lab_0x35, &&lab_0x36, &&lab_0x37,
&&lab_0x38, &&lab_0x39, &&lab_0x3a, &&lab_0x3b, &&lab_0x3c, &&lab_0x3d,
&&lab_0x3e, &&lab_0x3f, &&lab_0x40, &&lab_0x41, &&lab_0x42, &&lab_0x43,
&&lab_0x44, &&lab_0x45, &&lab_0x46, &&lab_0x47, &&lab_0x48, &&lab_0x49,
&&lab_0x4a, &&lab_0x4b, &&lab_0x4c, &&lab_0x4d, &&lab_0x4e, &&lab_0x4f,
&&lab_0x50, &&lab_0x51, &&lab_0x52, &&lab_0x53, &&lab_0x54, &&lab_0x55,
&&lab_0x56, &&lab_0x57, &&lab_0x58, &&lab_0x59, &&lab_0x5a, &&lab_0x5b,
&&lab_0x5c, &&lab_0x5d, &&lab_0x5e, &&lab_0x5f, &&lab_0x60, &&lab_0x61,
&&lab_0x62, &&lab_0x63, &&lab_0x64, &&lab_0x65, &&lab_0x66, &&lab_0x67,
&&lab_0x68, &&lab_0x69, &&lab_0x6a, &&lab_0x6b, &&lab_0x6c, &&lab_0x6d,
&&lab_0x6e, &&lab_0x6f, &&lab_0x70, &&lab_0x71, &&lab_0x72, &&lab_0x73,
&&lab_0x74, &&lab_0x75, &&lab_0x76, &&lab_0x77, &&lab_0x78, &&lab_0x79,
&&lab_0x7a, &&lab_0x7b, &&lab_0x7c, &&lab_0x7d, &&lab_0x7e, &&lab_0x7f,
&&lab_0x80, &&lab_0x81, &&lab_0x82, &&lab_0x83, &&lab_0x84, &&lab_0x85,
&&lab_0x86, &&lab_0x87, &&lab_0x88, &&lab_0x89, &&lab_0x8a, &&lab_0x8b,
&&lab_0x8c, &&lab_0x8d, &&lab_0x8e, &&lab_0x8f, &&lab_0x90, &&lab_0x91,
&&lab_0x92, &&lab_0x93, &&lab_0x94, &&lab_0x95, &&lab_0x96, &&lab_0x97,
&&lab_0x98, &&lab_0x99, &&lab_0x9a, &&lab_0x9b, &&lab_0x9c, &&lab_0x9d,
&&lab_0x9e, &&lab_0x9f, &&lab_0xa0, &&lab_0xa1, &&lab_0xa2, &&lab_0xa3,
&&lab_0xa4, &&lab_0xa5, &&lab_0xa6, &&lab_0xa7, &&lab_0xa8, &&lab_0xa9,
&&lab_0xaa, &&lab_0xab, &&lab_0xac, &&lab_0xad, &&lab_0xae, &&lab_0xaf,
&&lab_0xb0, &&lab_0xb1, &&lab_0xb2, &&lab_0xb3, &&lab_0xb4, &&lab_0xb5,
&&lab_0xb6, &&lab_0xb7, &&lab_0xb8, &&lab_0xb9, &&lab_0xba, &&lab_0xbb,
&&lab_0xbc, &&lab_0xbd, &&lab_0xbe, &&lab_0xbf, &&lab_0xc0, &&lab_0xc1,
&&lab_0xc2, &&lab_0xc3, &&lab_0xc4, &&lab_0xc5, &&lab_0xc6, &&lab_0xc7,
&&lab_0xc8, &&lab_0xc9, &&lab_0xca, &&lab_0xcb, &&lab_0xcc, &&lab_0xcd,
&&lab_0xce, &&lab_0xcf, &&lab_0xd0, &&lab_0xd1, &&lab_0xd2, &&lab_0xd3,
&&lab_0xd4, &&lab_0xd5, &&lab_0xd6, &&lab_0xd7, &&lab_0xd8, &&lab_0xd9,
&&lab_0xda, &&lab_0xdb, &&lab_0xdc, &&lab_0xdd, &&lab_0xde, &&lab_0xdf,
&&lab_0xe0, &&lab_0xe1, &&lab_0xe2, &&lab_0xe3, &&lab_0xe4, &&lab_0xe5,
&&lab_0xe6, &&lab_0xe7, &&lab_0xe8, &&lab_0xe9, &&lab_0xea, &&lab_0xeb,
&&lab_0xec, &&lab_0xed, &&lab_0xee, &&lab_0xef, &&lab_0xf0, &&lab_0xf1,
&&lab_0xf2, &&lab_0xf3, &&lab_0xf4, &&lab_0xf5, &&lab_0xf6, &&lab_0xf7,
&&lab_0xf8, &&lab_0xf9, &&lab_0xfa, &&lab_0xfb, &&lab_0xfc, &&lab_0xfd,
&&lab_0xfe, &&lab_0xff

};

  UChar I;
  UChar codop;
  UInt16 CycleNew = 0;

  goto lab_beg;

  {
lab_end:
    // HSYNC stuff - count cycles:
    if (cycles > V_HSYNC) {

        CycleNew += cycles;
	      /* Call the periodic handler */
        I = Loop6502 ();
        /* Reset the cycle counter */
        cycles = 0;
        if (I) Int6502(I);	/* Interrupt if needed  */


    } else
    if (CycleNew - cyclecountold >= TimerPeriod) {
      cyclecountold = CycleNew;
      I = TimerInt ();
      if (I) Int6502 (I);
    }

lab_beg:
   codop = PageR[reg_pc>>13][reg_pc];
# ifdef CODOP_PROFILE
   codop_profile[codop]++;
# endif
	 goto *a_jump_table[codop];
#include "h6280-switch.c"
  }
}
