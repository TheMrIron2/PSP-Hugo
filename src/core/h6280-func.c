static UChar adc(UChar acc, UChar val) 
{
  Int16  sig  = (Char)acc;
  UInt16 usig = (UChar)acc;
  UInt16 temp;

  if (!(reg_p & FL_D)) {		/* binary mode */
    if (reg_p & FL_C) {
      usig++;
      sig++;
    }
    sig  += (Char)val;
    usig += (UChar)val;
    acc   = (UChar)(usig & 0xFF);

    reg_p = (reg_p & ~(FL_N|FL_V|FL_T|FL_Z|FL_C))
            | (((sig > 127) || (sig < -128)) ? FL_V:0)
            | ((usig > 255) ? FL_C:0)
            | flnz_list[acc];

  } else {				/* decimal mode */

// treatment of out-of-range accumulator
// and operand values (non-BCD) is not
// adequately defined.	Nor is overflow
// flag treatment.

// Zeo : rewrote using bcdbin and binbcd arrays to boost code speed and fix
// residual bugs

    temp  = bcdbin[usig] + bcdbin[val];

    if (reg_p & FL_C) { temp++; }

    acc    = binbcd[temp];

    reg_p  = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C))
	     | ((temp > 99) ? FL_C:0)
	     | flnz_list[acc];

    cycles++;	/* decimal mode takes an extra cycle */

  }
  return(acc);
}

static void 
sbc(UChar val) 
{
  Int16  sig  = (Char)reg_a;
  UInt16 usig = (UChar)reg_a;
  Int16  temp;

  if (!(reg_p & FL_D)) {		/* binary mode */
    if (!(reg_p & FL_C)) {
      usig--;
      sig--;
    }
    sig   -= (Char)val;
    usig  -= (UChar)val;
    reg_a  = (UChar)(usig & 0xFF);
    reg_p  = (reg_p & ~(FL_N|FL_V|FL_T|FL_Z|FL_C))
	     | (((sig > 127) || (sig < -128)) ? FL_V:0)
	     | ((usig > 255) ? 0:FL_C)
	     | flnz_list[reg_a];      /* FL_N, FL_Z */

  } else {				/* decimal mode */

// treatment of out-of-range accumulator
// and operand values (non-bcd) is not
// adequately defined.	Nor is overflow
// flag treatment.

    temp  = (Int16)(bcdbin[usig] - bcdbin[val]);

    if (!(reg_p & FL_C)) { temp--; }

    reg_p  = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C))
	     | ((temp < 0) ? 0:FL_C);

    while (temp < 0) {
      temp += 100;
    }

    chk_flnz_8bit(reg_a = binbcd[temp]);

    cycles++;	/* decimal mode takes an extra cycle */

  }
}

static int 
adc_abs(void) 
{
// if flag 'T' is set, use zero-page address specified by register 'X'
// as the accumulator...

  if (reg_p & FL_T) {
    put_8bit_zp(reg_x, adc(get_8bit_zp(reg_x), abs_operand(reg_pc+1)));
    cycles+=8;
  } else {
    reg_a = adc(reg_a, abs_operand(reg_pc+1));
    cycles+=5;
  }
  reg_pc+=3;
  return 0;
}

static int 
adc_absx(void) 
{
  if (reg_p & FL_T) {
    put_8bit_zp(reg_x, adc(get_8bit_zp(reg_x), absx_operand(reg_pc+1)));
    cycles+=8;
  } else {
    reg_a = adc(reg_a, absx_operand(reg_pc+1));
    cycles+=5;
  }
  reg_pc+=3;
  return 0;
}

static int 
adc_absy(void) 
{
  if (reg_p & FL_T) {
    put_8bit_zp(reg_x, adc(get_8bit_zp(reg_x), absy_operand(reg_pc+1)));
    cycles+=8;
  } else {
    reg_a = adc(reg_a, absy_operand(reg_pc+1));
    cycles+=5;
  }
  reg_pc+=3;
  return 0;
}

static inline int adc_imm(void) {
  if (reg_p & FL_T) {
    put_8bit_zp(reg_x, adc(get_8bit_zp(reg_x), imm_operand(reg_pc+1)));
    cycles+=5;
  } else {
    reg_a = adc(reg_a, imm_operand(reg_pc+1));
    cycles+=2;
  }
  reg_pc+=2;
  return 0;
}

static inline int adc_zp(void) {
  if (reg_p & FL_T) {
    put_8bit_zp(reg_x, adc(get_8bit_zp(reg_x), zp_operand(reg_pc+1)));
    cycles+=7;
  } else {
    reg_a = adc(reg_a, zp_operand(reg_pc+1));
    cycles+=4;
  }
  reg_pc+=2;
  return 0;
}

static inline int adc_zpx(void) {
  if (reg_p & FL_T) {
    put_8bit_zp(reg_x, adc(get_8bit_zp(reg_x), zpx_operand(reg_pc+1)));
    cycles+=7;
  } else {
    reg_a = adc(reg_a, zpx_operand(reg_pc+1));
    cycles+=4;
  }
  reg_pc+=2;
  return 0;
}

static inline int adc_zpind(void) {
  if (reg_p & FL_T) {
    put_8bit_zp(reg_x, adc(get_8bit_zp(reg_x), zpind_operand(reg_pc+1)));
    cycles+=10;
  } else {
    reg_a = adc(reg_a, zpind_operand(reg_pc+1));
    cycles+=7;
  }
  reg_pc+=2;
  return 0;
}

static inline int adc_zpindx(void) {
  if (reg_p & FL_T) {
    put_8bit_zp(reg_x, adc(get_8bit_zp(reg_x), zpindx_operand(reg_pc+1)));
    cycles+=10;
  } else {
    reg_a = adc(reg_a, zpindx_operand(reg_pc+1));
    cycles+=7;
  }
  reg_pc+=2;
  return 0;
}

#define adc_zpindy() \
  do { \
  if (reg_p & FL_T) { \
    put_8bit_zp(reg_x, adc(get_8bit_zp(reg_x), zpindy_operand(reg_pc+1))); \
    cycles+=10; \
  } else { \
    reg_a = adc(reg_a, zpindy_operand(reg_pc+1)); \
    cycles+=7; \
  } \
  reg_pc+=2; \
  } while (0)

#define and_abs() \
  do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp &= abs_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=8; \
 \
  } else { \
    chk_flnz_8bit(reg_a &= abs_operand(reg_pc+1)); \
    cycles+=5; \
  } \
  reg_pc+=3; \
  } while (0)

#define and_absx() \
  do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp &= absx_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=8; \
  \
  } else {  \
    chk_flnz_8bit(reg_a &= absx_operand(reg_pc+1));  \
    cycles+=5;  \
  }  \
  reg_pc+=3;  \
  } while (0)

#define and_absy() \
  do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp &= absy_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=8; \
  } else { \
    chk_flnz_8bit(reg_a &= absy_operand(reg_pc+1)); \
    cycles+=5; \
  } \
  reg_pc+=3; \
  } while (0)

#define and_imm() \
  do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp &= imm_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=5; \
 \
  } else { \
    chk_flnz_8bit(reg_a &= imm_operand(reg_pc+1)); \
    cycles+=2; \
  } \
  reg_pc+=2; \
  } while (0)

#define and_zp() \
  do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp &= zp_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=7; \
 \
  } else { \
    chk_flnz_8bit(reg_a &= zp_operand(reg_pc+1)); \
    cycles+=4; \
  } \
  reg_pc+=2; \
  } while (0)

#define and_zpx() \
  do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp &= zpx_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=7; \
 \
  } else { \
    chk_flnz_8bit(reg_a &= zpx_operand(reg_pc+1)); \
    cycles+=4; \
  } \
  reg_pc+=2; \
  } while (0)

#define and_zpind() \
  do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp &= zpind_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=10; \
  } else { \
    chk_flnz_8bit(reg_a &= zpind_operand(reg_pc+1)); \
    cycles+=7; \
  } \
  reg_pc+=2; \
  } while (0)

# define  and_zpindx() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp &= zpindx_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=10; \
 \
  } else { \
    chk_flnz_8bit(reg_a &= zpindx_operand(reg_pc+1)); \
    cycles+=7; \
  } \
  reg_pc+=2; \
  } while (0)


# define  and_zpindy() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp &= zpindy_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=10; \
 \
  } else { \
    chk_flnz_8bit(reg_a &= zpindy_operand(reg_pc+1)); \
    cycles+=7; \
  } \
  reg_pc+=2; \
  } while (0)


# define  asl_a() do { \
  UChar temp1 = reg_a; \
  reg_a<<=1; \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp1 & 0x80) ? FL_C:0) \
	  | flnz_list[reg_a]; \
  cycles+=2; \
  reg_pc++; \
  } while (0)


# define  asl_abs() do { \
  UInt16 temp_addr = get_16bit_addr(reg_pc+1); \
  UChar  temp1	   = get_8bit_addr(temp_addr); \
  UChar  temp	   = temp1<<1; \
  \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp1 & 0x80) ? FL_C:0) \
	  | flnz_list[temp]; \
  cycles+=7; \
  \
  put_8bit_addr(temp_addr,temp); \
  reg_pc+=3; \
  } while (0)

# define  asl_absx() do { \
  UInt16 temp_addr = get_16bit_addr(reg_pc+1)+reg_x; \
  UChar  temp1	   = get_8bit_addr(temp_addr); \
  UChar  temp	   = temp1<<1; \
  \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp1 & 0x80) ? FL_C:0) \
	  | flnz_list[temp]; \
  cycles+=7; \
  put_8bit_addr(temp_addr,temp); \
  reg_pc+=3; \
  } while (0)


# define  asl_zp() do { \
  UChar zp_addr = imm_operand(reg_pc+1); \
  UChar temp1	= get_8bit_zp(zp_addr); \
  UChar temp	= temp1<<1; \
  \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp1 & 0x80) ? FL_C:0) \
	  | flnz_list[temp]; \
  cycles+=6; \
  put_8bit_zp(zp_addr, temp); \
  reg_pc+=2; \
  } while (0)


# define  asl_zpx() do { \
  UChar  zp_addr = imm_operand(reg_pc+1)+reg_x; \
  UChar  temp1	 = get_8bit_zp(zp_addr); \
  UChar  temp	 = temp1<<1; \
  \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp1 & 0x80) ? FL_C:0) \
	  | flnz_list[temp]; \
  cycles+=6; \
  put_8bit_zp(zp_addr, temp); \
  reg_pc+=2; \
  } while (0)


# define  bbr0() do { \
  reg_p &= ~FL_T; \
  if (zp_operand(reg_pc+1)&0x01) { \
    reg_pc+=3; \
    cycles+=6; \
  } else { \
    reg_pc+=(Char)imm_operand(reg_pc+2)+3; \
    cycles+=8; \
  } \
  } while (0)


# define  bbr1() do { \
  reg_p &= ~FL_T; \
  if (zp_operand(reg_pc+1)&0x02) { \
    reg_pc+=3; \
    cycles+=6; \
  } else { \
    reg_pc+=(Char)imm_operand(reg_pc+2)+3; \
    cycles+=8; \
  } \
  } while (0)

# define  bbr2() do { \
  reg_p &= ~FL_T; \
  if (zp_operand(reg_pc+1)&0x04) { \
    reg_pc+=3; \
    cycles+=6; \
  } else { \
    reg_pc+=(Char)imm_operand(reg_pc+2)+3; \
    cycles+=8; \
  } \
  } while (0)

# define  bbr3() do { \
  reg_p &= ~FL_T; \
  if (zp_operand(reg_pc+1)&0x08) { \
    reg_pc+=3; \
    cycles+=6; \
  } else { \
    reg_pc+=(Char)imm_operand(reg_pc+2)+3; \
    cycles+=8; \
  } \
  } while (0)


# define  bbr4() do { \
  reg_p &= ~FL_T; \
  if (zp_operand(reg_pc+1)&0x10) { \
    reg_pc+=3; \
    cycles+=6; \
  } else { \
    reg_pc+=(Char)imm_operand(reg_pc+2)+3; \
    cycles+=8; \
  } \
  } while (0)


# define  bbr5() do { \
  reg_p &= ~FL_T; \
  if (zp_operand(reg_pc+1)&0x20) { \
    reg_pc+=3; \
    cycles+=6; \
  } else { \
    reg_pc+=(Char)imm_operand(reg_pc+2)+3; \
    cycles+=8; \
  } \
  } while (0)


# define  bbr6() do { \
  reg_p &= ~FL_T; \
  if (zp_operand(reg_pc+1)&0x40) { \
    reg_pc+=3; \
    cycles+=6; \
  } else { \
    reg_pc+=(Char)imm_operand(reg_pc+2)+3; \
    cycles+=8; \
  } \
  } while (0)


# define  bbr7() do { \
  reg_p &= ~FL_T; \
  if (zp_operand(reg_pc+1)&0x80) { \
    reg_pc+=3; \
    cycles+=6; \
  } else { \
    reg_pc+=(Char)imm_operand(reg_pc+2)+3; \
    cycles+=8; \
  } \
  } while (0)


# define  bbs0() do { \
  reg_p &= ~FL_T; \
  if (zp_operand(reg_pc+1)&0x01) { \
    reg_pc+=(Char)imm_operand(reg_pc+2)+3; \
    cycles+=8; \
  } else { \
    reg_pc+=3; \
    cycles+=6; \
  } \
  } while (0)


# define  bbs1() do { \
  reg_p &= ~FL_T; \
  if (zp_operand(reg_pc+1)&0x02) { \
    reg_pc+=(Char)imm_operand(reg_pc+2)+3; \
    cycles+=8; \
  } else { \
    reg_pc+=3; \
    cycles+=6; \
  } \
  } while (0)


# define  bbs2() do { \
  reg_p &= ~FL_T; \
  if (zp_operand(reg_pc+1)&0x04) { \
    reg_pc+=(Char)imm_operand(reg_pc+2)+3; \
    cycles+=8; \
  } else { \
    reg_pc+=3; \
    cycles+=6; \
  } \
  } while (0)


# define  bbs3() do { \
  reg_p &= ~FL_T; \
  if (zp_operand(reg_pc+1)&0x08) { \
    reg_pc+=(Char)imm_operand(reg_pc+2)+3; \
    cycles+=8; \
  } else { \
    reg_pc+=3; \
    cycles+=6; \
  } \
  } while (0)


# define  bbs4() do { \
  reg_p &= ~FL_T; \
  if (zp_operand(reg_pc+1)&0x10) { \
    reg_pc+=(Char)imm_operand(reg_pc+2)+3; \
    cycles+=8; \
  } else { \
    reg_pc+=3; \
    cycles+=6; \
  } \
  } while (0)


# define  bbs5() do { \
  reg_p &= ~FL_T; \
  if (zp_operand(reg_pc+1)&0x20) { \
    reg_pc+=(Char)imm_operand(reg_pc+2)+3; \
    cycles+=8; \
  } else { \
    reg_pc+=3; \
    cycles+=6; \
  } \
  } while (0)


# define  bbs6() do { \
  reg_p &= ~FL_T; \
  if (zp_operand(reg_pc+1)&0x40) { \
    reg_pc+=(Char)imm_operand(reg_pc+2)+3; \
    cycles+=8; \
  } else { \
    reg_pc+=3; \
    cycles+=6; \
  } \
  } while (0)


# define  bbs7() do { \
  reg_p &= ~FL_T; \
  if (zp_operand(reg_pc+1)&0x80) { \
    reg_pc+=(Char)imm_operand(reg_pc+2)+3; \
    cycles+=8; \
  } else { \
    reg_pc+=3; \
    cycles+=6; \
  } \
  } while (0)


# define  bcc() do { \
  reg_p &= ~FL_T; \
  if (reg_p & FL_C) { \
    reg_pc+=2; \
    cycles+=2; \
  } else { \
    reg_pc+=(Char)imm_operand(reg_pc+1)+2; \
    cycles+=4; \
  } \
  } while (0)


# define  bcs() do { \
  reg_p &= ~FL_T; \
  if (reg_p & FL_C) { \
    reg_pc+=(Char)imm_operand(reg_pc+1)+2; \
    cycles+=4; \
  } else { \
    reg_pc+=2; \
    cycles+=2; \
  } \
  } while (0)


# define  beq() do { \
  reg_p &= ~FL_T; \
  if (reg_p & FL_Z) { \
    reg_pc+=(Char)imm_operand(reg_pc+1)+2; \
    cycles+=4; \
  } else { \
    reg_pc+=2; \
    cycles+=2; \
  } \
  } while (0)


# define  bit_abs() do { \
  UChar temp = abs_operand(reg_pc+1); \
  reg_p = (reg_p & ~(FL_N|FL_V|FL_T|FL_Z)) \
	  | ((temp&0x80)  ? FL_N:0) \
	  | ((temp&0x40)  ? FL_V:0) \
	  | ((reg_a&temp) ? 0:FL_Z); \
  reg_pc+=3; \
  cycles+=5; \
  } while (0)


# define  bit_absx() do { \
  UChar temp = absx_operand(reg_pc+1); \
  reg_p = (reg_p & ~(FL_N|FL_V|FL_T|FL_Z)) \
	  | ((temp&0x80)  ? FL_N:0) \
	  | ((temp&0x40)  ? FL_V:0) \
	  | ((reg_a&temp) ? 0:FL_Z); \
  reg_pc+=3; \
  cycles+=5; \
  } while (0)


# define  bit_imm() do { \
 \
  UChar temp = imm_operand(reg_pc+1); \
  reg_p = (reg_p & ~(FL_N|FL_V|FL_T|FL_Z)) \
	  | ((temp&0x80)  ? FL_N:0) \
	  | ((temp&0x40)  ? FL_V:0) \
	  | ((reg_a&temp) ? 0:FL_Z); \
  reg_pc+=2; \
  cycles+=2; \
  } while (0)


# define  bit_zp() do { \
  UChar temp = zp_operand(reg_pc+1); \
  reg_p = (reg_p & ~(FL_N|FL_V|FL_T|FL_Z)) \
	  | ((temp&0x80)  ? FL_N:0) \
	  | ((temp&0x40)  ? FL_V:0) \
	  | ((reg_a&temp) ? 0:FL_Z); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  bit_zpx() do { \
  UChar temp = zpx_operand(reg_pc+1); \
  reg_p = (reg_p & ~(FL_N|FL_V|FL_T|FL_Z)) \
	  | ((temp&0x80)  ? FL_N:0) \
	  | ((temp&0x40)  ? FL_V:0) \
	  | ((reg_a&temp) ? 0:FL_Z); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  bmi() do { \
  reg_p &= ~FL_T; \
  if (reg_p & FL_N) { \
    reg_pc+=(Char)imm_operand(reg_pc+1)+2; \
    cycles+=4; \
  } else { \
    reg_pc+=2; \
    cycles+=2; \
  } \
  } while (0)


# define  bne() do { \
  reg_p &= ~FL_T; \
  if (reg_p & FL_Z) { \
    reg_pc+=2; \
    cycles+=2; \
  } else { \
    reg_pc+=(Char)imm_operand(reg_pc+1)+2; \
    cycles+=4; \
  } \
  } while (0)


# define  bpl() do { \
  reg_p &= ~FL_T; \
  if (reg_p & FL_N) { \
    reg_pc+=2; \
    cycles+=2; \
  } else { \
    reg_pc+=(Char)imm_operand(reg_pc+1)+2; \
    cycles+=4; \
  } \
  } while (0)


# define  bra() do { \
  reg_p &= ~FL_T; \
  reg_pc+=(Char)imm_operand(reg_pc+1)+2; \
  cycles+=4; \
  } while (0)


# define  brek() do { \
  push_16bit(reg_pc+2); \
  reg_p &= ~FL_T; \
  push_8bit(reg_p|FL_B); \
  reg_p =(reg_p & ~FL_D) | FL_I; \
  reg_pc=get_16bit_addr(0xFFF6); \
  cycles+=8; \
  } while (0)


# define  bsr() do { \
  reg_p &= ~FL_T; \
  push_16bit(reg_pc+1); \
  reg_pc+=(Char)imm_operand(reg_pc+1)+2; \
  cycles+=8; \
  } while (0)


# define  bvc() do { \
  reg_p &= ~FL_T; \
  if (reg_p & FL_V) { \
    reg_pc+=2; \
    cycles+=2; \
  } else { \
    reg_pc+=(Char)imm_operand(reg_pc+1)+2; \
    cycles+=4; \
  } \
  } while (0)


# define  bvs() do { \
  reg_p &= ~FL_T; \
  if (reg_p & FL_V) { \
    reg_pc+=(Char)imm_operand(reg_pc+1)+2; \
    cycles+=4; \
  } else { \
    reg_pc+=2; \
    cycles+=2; \
  } \
  } while (0)


# define  cla() do { \
  reg_p &= ~FL_T; \
  reg_a = 0; \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  clc() do { \
  reg_p &= ~(FL_T|FL_C); \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  cld() do { \
  reg_p &= ~(FL_T|FL_D); \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  cli() do { \
  reg_p &= ~(FL_T|FL_I); \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  clv() do { \
  reg_p &= ~(FL_V|FL_T); \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  clx() do { \
  reg_p &= ~FL_T; \
  reg_x = 0; \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  cly() do { \
  reg_p &= ~FL_T; \
  reg_y = 0; \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  cmp_abs() do { \
  UChar temp = abs_operand(reg_pc+1); \
  \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((reg_a < temp) ? 0:FL_C) \
	  | flnz_list[(UChar)(reg_a-temp)]; \
  reg_pc+=3; \
  cycles+=5; \
  } while (0)


# define  cmp_absx() do { \
  UChar temp = absx_operand(reg_pc+1); \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((reg_a < temp) ? 0:FL_C) \
	  | flnz_list[(UChar)(reg_a-temp)]; \
  reg_pc+=3; \
  cycles+=5; \
  } while (0)

# define  cmp_absy() do { \
  UChar temp = absy_operand(reg_pc+1); \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((reg_a < temp) ? 0:FL_C) \
	  | flnz_list[(UChar)(reg_a-temp)]; \
  reg_pc+=3; \
  cycles+=5; \
  } while (0)

# define  cmp_imm() do { \
  UChar temp = imm_operand(reg_pc+1); \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((reg_a < temp) ? 0:FL_C) \
	  | flnz_list[(UChar)(reg_a-temp)]; \
  reg_pc+=2; \
  cycles+=2; \
  } while (0)


# define cmp_zp() \
  do { \
  UChar temp = zp_operand(reg_pc+1); \
  \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((reg_a < temp) ? 0:FL_C) \
	  | flnz_list[(UChar)(reg_a-temp)]; \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)

# define  cmp_zpx() do { \
  UChar temp = zpx_operand(reg_pc+1); \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((reg_a < temp) ? 0:FL_C) \
	  | flnz_list[(UChar)(reg_a-temp)]; \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  cmp_zpind() do { \
  UChar temp = zpind_operand(reg_pc+1); \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((reg_a < temp) ? 0:FL_C) \
	  | flnz_list[(UChar)(reg_a-temp)]; \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  cmp_zpindx() do { \
  UChar temp = zpindx_operand(reg_pc+1); \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((reg_a < temp) ? 0:FL_C) \
	  | flnz_list[(UChar)(reg_a-temp)]; \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  cmp_zpindy() do { \
  UChar temp = zpindy_operand(reg_pc+1); \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((reg_a < temp) ? 0:FL_C) \
	  | flnz_list[(UChar)(reg_a-temp)]; \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  cpx_abs() do { \
  UChar temp = abs_operand(reg_pc+1); \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((reg_x < temp) ? 0:FL_C) \
	  | flnz_list[(UChar)(reg_x-temp)]; \
  reg_pc+=3; \
  cycles+=5; \
  } while (0)


# define  cpx_imm() do { \
  UChar temp = imm_operand(reg_pc+1); \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((reg_x < temp) ? 0:FL_C) \
	  | flnz_list[(UChar)(reg_x-temp)]; \
  reg_pc+=2; \
  cycles+=2; \
  } while (0)


# define  cpx_zp() do { \
  UChar temp = zp_operand(reg_pc+1); \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((reg_x < temp) ? 0:FL_C) \
	  | flnz_list[(UChar)(reg_x-temp)]; \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  cpy_abs() do { \
  UChar temp = abs_operand(reg_pc+1); \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((reg_y < temp) ? 0:FL_C) \
	  | flnz_list[(UChar)(reg_y-temp)]; \
  reg_pc+=3; \
  cycles+=5; \
  } while (0)


# define  cpy_imm() do { \
  UChar temp = imm_operand(reg_pc+1); \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((reg_y < temp) ? 0:FL_C) \
	  | flnz_list[(UChar)(reg_y-temp)]; \
  reg_pc+=2; \
  cycles+=2; \
  } while (0)


# define  cpy_zp() do { \
  UChar temp = zp_operand(reg_pc+1); \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((reg_y < temp) ? 0:FL_C) \
	  | flnz_list[(UChar)(reg_y-temp)]; \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  dec_a() do { \
  chk_flnz_8bit(--reg_a); \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  dec_abs() do { \
  UChar  temp; \
  UInt16 temp_addr = get_16bit_addr(reg_pc+1); \
  chk_flnz_8bit(temp = get_8bit_addr(temp_addr)-1); \
  cycles+=7; \
  put_8bit_addr(temp_addr, temp); \
  reg_pc+=3; \
  } while (0)


# define  dec_absx() do { \
  UChar  temp; \
  UInt16 temp_addr = get_16bit_addr(reg_pc+1)+reg_x; \
  chk_flnz_8bit(temp = get_8bit_addr(temp_addr)-1); \
  cycles+=7; \
  put_8bit_addr(temp_addr, temp); \
  reg_pc+=3; \
  } while (0)


# define  dec_zp() do { \
  UChar  temp; \
  UChar  zp_addr = imm_operand(reg_pc+1); \
  chk_flnz_8bit(temp = get_8bit_zp(zp_addr)-1); \
  put_8bit_zp(zp_addr, temp); \
  reg_pc+=2; \
  cycles+=6; \
  } while (0)


# define  dec_zpx() do { \
  UChar  temp; \
  UChar  zp_addr = imm_operand(reg_pc+1)+reg_x; \
  chk_flnz_8bit(temp = get_8bit_zp(zp_addr)-1); \
  put_8bit_zp(zp_addr, temp); \
  reg_pc+=2; \
  cycles+=6; \
  } while (0)


# define  dex() do { \
  chk_flnz_8bit(--reg_x); \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  dey() do { \
  chk_flnz_8bit(--reg_y); \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  eor_abs() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp ^= abs_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=8; \
 \
  } else { \
    chk_flnz_8bit(reg_a ^= abs_operand(reg_pc+1)); \
    cycles+=5; \
  } \
  reg_pc+=3; \
  } while (0)


# define  eor_absx() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp ^= absx_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=8; \
 \
  } else { \
    chk_flnz_8bit(reg_a ^= absx_operand(reg_pc+1)); \
    cycles+=5; \
  } \
  reg_pc+=3; \
  } while (0)


# define  eor_absy() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp ^= absy_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=8; \
 \
  } else { \
    chk_flnz_8bit(reg_a ^= absy_operand(reg_pc+1)); \
    cycles+=5; \
  } \
  reg_pc+=3; \
  } while (0)


# define  eor_imm() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp ^= imm_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=5; \
 \
  } else { \
    chk_flnz_8bit(reg_a ^= imm_operand(reg_pc+1)); \
    cycles+=2; \
  } \
  reg_pc+=2; \
  } while (0)


# define  eor_zp() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp ^= zp_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=7; \
 \
  } else { \
    chk_flnz_8bit(reg_a ^= zp_operand(reg_pc+1)); \
    cycles+=4; \
  } \
  reg_pc+=2; \
  } while (0)


# define  eor_zpx() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp ^= zpx_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=7; \
 \
  } else { \
    chk_flnz_8bit(reg_a ^= zpx_operand(reg_pc+1)); \
    cycles+=4; \
  } \
  reg_pc+=2; \
  } while (0)


# define  eor_zpind() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp ^= zpind_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=10; \
 \
  } else { \
    chk_flnz_8bit(reg_a ^= zpind_operand(reg_pc+1)); \
    cycles+=7; \
  } \
  reg_pc+=2; \
  } while (0)


# define  eor_zpindx() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp ^= zpindx_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=10; \
 \
  } else { \
    chk_flnz_8bit(reg_a ^= zpindx_operand(reg_pc+1)); \
    cycles+=7; \
  } \
  reg_pc+=2; \
  } while (0)


# define  eor_zpindy() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp ^= zpindy_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=10; \
 \
  } else { \
    chk_flnz_8bit(reg_a ^= zpindy_operand(reg_pc+1)); \
    cycles+=7; \
  } \
  reg_pc+=2; \
  } while (0)


# define  halt() do { \
  return; \
  } while (0)


# define  inc_a() do { \
  chk_flnz_8bit(++reg_a); \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  inc_abs() do { \
  UChar  temp; \
  UInt16 temp_addr = get_16bit_addr(reg_pc+1); \
  chk_flnz_8bit(temp = get_8bit_addr(temp_addr)+1); \
  cycles+=7; \
  put_8bit_addr(temp_addr, temp); \
  reg_pc+=3; \
  } while (0)


# define  inc_absx() do { \
  UChar  temp; \
  UInt16 temp_addr = get_16bit_addr(reg_pc+1)+reg_x; \
  chk_flnz_8bit(temp = get_8bit_addr(temp_addr)+1); \
  cycles+=7; \
  put_8bit_addr(temp_addr, temp); \
  reg_pc+=3; \
  } while (0)


# define  inc_zp() do { \
  UChar temp; \
  UChar zp_addr = imm_operand(reg_pc+1); \
  chk_flnz_8bit(temp = get_8bit_zp(zp_addr)+1); \
  put_8bit_zp(zp_addr, temp); \
 \
  reg_pc+=2; \
  cycles+=6; \
  } while (0)


# define  inc_zpx() do { \
  UChar temp; \
  UChar zp_addr = imm_operand(reg_pc+1)+reg_x; \
  chk_flnz_8bit(temp = get_8bit_zp(zp_addr)+1); \
  put_8bit_zp(zp_addr, temp); \
  reg_pc+=2; \
  cycles+=6; \
  } while (0)


# define  inx() do { \
  chk_flnz_8bit(++reg_x); \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  iny() do { \
  chk_flnz_8bit(++reg_y); \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  jmp() do { \
  reg_p &= ~FL_T; \
  reg_pc = get_16bit_addr(reg_pc+1); \
  cycles+=4; \
  } while (0)


# define  jmp_absind() do { \
  reg_p &= ~FL_T; \
  reg_pc = get_16bit_addr(get_16bit_addr(reg_pc+1)); \
  cycles+=7; \
  } while (0)


# define  jmp_absindx() do { \
  reg_p &= ~FL_T; \
  reg_pc = get_16bit_addr(get_16bit_addr(reg_pc+1)+reg_x); \
  cycles+=7; \
  } while (0)


# define  jsr() do { \
  reg_p &= ~FL_T; \
  push_16bit(reg_pc+2); \
  reg_pc = get_16bit_addr(reg_pc+1); \
  cycles+=7; \
  } while (0)


# define  lda_abs() do { \
  chk_flnz_8bit(reg_a = abs_operand(reg_pc+1)); \
  reg_pc+=3; \
  cycles+=5; \
  } while (0)


# define  lda_absx() do { \
  chk_flnz_8bit(reg_a = absx_operand(reg_pc+1)); \
  reg_pc+=3; \
  cycles+=5; \
  } while (0)


# define  lda_absy() do { \
  chk_flnz_8bit(reg_a = absy_operand(reg_pc+1)); \
  reg_pc+=3; \
  cycles+=5; \
  } while (0)


# define  lda_imm() do { \
  chk_flnz_8bit(reg_a = imm_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=2; \
  } while (0)


# define  lda_zp() do { \
  chk_flnz_8bit(reg_a = zp_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  lda_zpx() do { \
  chk_flnz_8bit(reg_a = zpx_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  lda_zpind() do { \
  chk_flnz_8bit(reg_a = zpind_operand(reg_pc+1)); \
 \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  lda_zpindx() do { \
  chk_flnz_8bit(reg_a = zpindx_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  lda_zpindy() do { \
  chk_flnz_8bit(reg_a = zpindy_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  ldx_abs() do { \
  chk_flnz_8bit(reg_x = abs_operand(reg_pc+1)); \
  reg_pc+=3; \
  cycles+=5; \
  } while (0)


# define  ldx_absy() do { \
  chk_flnz_8bit(reg_x = absy_operand(reg_pc+1)); \
  reg_pc+=3; \
  cycles+=5; \
  } while (0)


# define  ldx_imm() do { \
  chk_flnz_8bit(reg_x = imm_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=2; \
  } while (0)


# define  ldx_zp() do { \
  chk_flnz_8bit(reg_x = zp_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  ldx_zpy() do { \
  chk_flnz_8bit(reg_x = zpy_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  ldy_abs() do { \
  chk_flnz_8bit(reg_y = abs_operand(reg_pc+1)); \
  reg_pc+=3; \
  cycles+=5; \
  } while (0)


# define  ldy_absx() do { \
  chk_flnz_8bit(reg_y = absx_operand(reg_pc+1)); \
  reg_pc+=3; \
  cycles+=5; \
  } while (0)


# define  ldy_imm() do { \
  chk_flnz_8bit(reg_y = imm_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=2; \
  } while (0)


# define  ldy_zp() do { \
  chk_flnz_8bit(reg_y = zp_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  ldy_zpx() do { \
  chk_flnz_8bit(reg_y = zpx_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  lsr_a() do { \
  UChar temp = reg_a; \
  reg_a/=2; \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp&1) ? FL_C:0) \
	  | flnz_list[reg_a]; \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  lsr_abs() do { \
  UInt16 temp_addr = get_16bit_addr(reg_pc+1); \
  UChar  temp1	   = get_8bit_addr(temp_addr); \
  UChar  temp	   = temp1/2; \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp1&1) ? FL_C:0) \
	  | flnz_list[temp]; \
  cycles+=7; \
  put_8bit_addr(temp_addr, temp); \
  reg_pc+=3; \
  } while (0)


# define  lsr_absx() do { \
  UInt16 temp_addr = get_16bit_addr(reg_pc+1)+reg_x; \
  UChar  temp1	   = get_8bit_addr(temp_addr); \
  UChar  temp	   = temp1/2; \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp1&1) ? FL_C:0) \
	  | flnz_list[temp]; \
  cycles+=7; \
  put_8bit_addr(temp_addr, temp); \
  reg_pc+=3; \
  } while (0)


# define  lsr_zp() do { \
  UChar  zp_addr = imm_operand(reg_pc+1); \
  UChar  temp1	 = get_8bit_zp(zp_addr); \
  UChar  temp	 = temp1/2; \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp1&1) ? FL_C:0) \
	  | flnz_list[temp]; \
  put_8bit_zp(zp_addr, temp); \
  reg_pc+=2; \
  cycles+=6; \
  } while (0)


# define  lsr_zpx() do { \
  UChar  zp_addr = imm_operand(reg_pc+1)+reg_x; \
  UChar  temp1	 = get_8bit_zp(zp_addr); \
  UChar  temp	 = temp1/2; \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp1&1) ? FL_C:0) \
	  | flnz_list[temp]; \
  put_8bit_zp(zp_addr, temp); \
  reg_pc+=2; \
  cycles+=6; \
  } while (0)


# define  nop(void)  { \
  reg_p &= ~FL_T; \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  ora_abs() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp |= abs_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=8; \
 \
  } else { \
    chk_flnz_8bit(reg_a |= abs_operand(reg_pc+1)); \
    cycles+=5; \
  } \
  reg_pc+=3; \
  } while (0)


# define  ora_absx() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp |= absx_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=8; \
 \
  } else { \
    chk_flnz_8bit(reg_a |= absx_operand(reg_pc+1)); \
    cycles+=5; \
  } \
  reg_pc+=3; \
  } while (0)


# define  ora_absy() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp |= absy_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=8; \
 \
  } else { \
    chk_flnz_8bit(reg_a |= absy_operand(reg_pc+1)); \
    cycles+=5; \
  } \
  reg_pc+=3; \
  } while (0)


# define  ora_imm() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp |= imm_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=5; \
 \
  } else { \
    chk_flnz_8bit(reg_a |= imm_operand(reg_pc+1)); \
    cycles+=2; \
  } \
  reg_pc+=2; \
  } while (0)


# define  ora_zp() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp |= zp_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=7; \
 \
  } else { \
    chk_flnz_8bit(reg_a |= zp_operand(reg_pc+1)); \
    cycles+=4; \
  } \
  reg_pc+=2; \
  } while (0)


# define  ora_zpx() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp |= zpx_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=7; \
 \
  } else { \
    chk_flnz_8bit(reg_a |= zpx_operand(reg_pc+1)); \
    cycles+=4; \
  } \
  reg_pc+=2; \
  } while (0)


# define  ora_zpind() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp |= zpind_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=10; \
 \
  } else { \
    chk_flnz_8bit(reg_a |= zpind_operand(reg_pc+1)); \
    cycles+=7; \
  } \
  reg_pc+=2; \
  } while (0)


# define  ora_zpindx() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp |= zpindx_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=10; \
 \
  } else { \
    chk_flnz_8bit(reg_a |= zpindx_operand(reg_pc+1)); \
    cycles+=7; \
  } \
  reg_pc+=2; \
  } while (0)


# define  ora_zpindy() do { \
  if (reg_p & FL_T) { \
    UChar temp = get_8bit_zp(reg_x); \
    chk_flnz_8bit(temp |= zpindy_operand(reg_pc+1)); \
    put_8bit_zp(reg_x, temp); \
    cycles+=10; \
 \
  } else { \
    chk_flnz_8bit(reg_a |= zpindy_operand(reg_pc+1)); \
    cycles+=7; \
  } \
  reg_pc+=2; \
  } while (0)


# define  pha() do { \
  reg_p &= ~FL_T; \
  push_8bit(reg_a); \
  reg_pc++; \
  cycles+=3; \
  } while (0)


# define  php() do { \
  reg_p &= ~FL_T; \
  push_8bit(reg_p); \
  reg_pc++; \
  cycles+=3; \
  } while (0)


# define  phx() do { \
  reg_p &= ~FL_T; \
  push_8bit(reg_x); \
  reg_pc++; \
  cycles+=3; \
  } while (0)


# define  phy() do { \
  reg_p &= ~FL_T; \
  push_8bit(reg_y); \
  reg_pc++; \
  cycles+=3; \
  } while (0)


# define  pla() do { \
  chk_flnz_8bit(reg_a = pull_8bit()); \
  reg_pc++; \
  cycles+=4; \
  } while (0)


# define  plp() do { \
  reg_p = pull_8bit(); \
  reg_pc++; \
  cycles+=4; \
  } while (0)


# define  plx() do { \
  chk_flnz_8bit(reg_x = pull_8bit()); \
  reg_pc++; \
  cycles+=4; \
  } while (0)


# define  ply() do { \
  chk_flnz_8bit(reg_y = pull_8bit()); \
  reg_pc++; \
  cycles+=4; \
  } while (0)


# define  rmb0() do { \
  UChar temp = imm_operand(reg_pc+1); \
  reg_p &= ~FL_T; \
  put_8bit_zp(temp, get_8bit_zp(temp) & (~0x01)); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  rmb1() do { \
  UChar temp = imm_operand(reg_pc+1); \
  reg_p &= ~FL_T; \
  put_8bit_zp(temp, get_8bit_zp(temp) & (~0x02)); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  rmb2() do { \
  UChar temp = imm_operand(reg_pc+1); \
  reg_p &= ~FL_T; \
  put_8bit_zp(temp, get_8bit_zp(temp) & (~0x04)); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  rmb3() do { \
  UChar temp = imm_operand(reg_pc+1); \
  reg_p &= ~FL_T; \
  put_8bit_zp(temp, get_8bit_zp(temp) & (~0x08)); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  rmb4() do { \
  UChar temp = imm_operand(reg_pc+1); \
  reg_p &= ~FL_T; \
  put_8bit_zp(temp, get_8bit_zp(temp) & (~0x10)); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  rmb5() do { \
  UChar temp = imm_operand(reg_pc+1); \
  reg_p &= ~FL_T; \
  put_8bit_zp(temp, get_8bit_zp(temp) & (~0x20)); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  rmb6() do { \
  UChar temp = imm_operand(reg_pc+1); \
  reg_p &= ~FL_T; \
  put_8bit_zp(temp, get_8bit_zp(temp) & (~0x40)); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  rmb7() do { \
  UChar temp = imm_operand(reg_pc+1); \
  reg_p &= ~FL_T; \
  put_8bit_zp(temp, get_8bit_zp(temp) & (~0x80)); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  rol_a() do { \
  UChar flg_tmp = (reg_p & FL_C) ? 1:0; \
  UChar temp = reg_a; \
 \
  reg_a = (reg_a<<1)+flg_tmp; \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp & 0x80) ? FL_C:0) \
	  | flnz_list[reg_a]; \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  rol_abs() do { \
  UChar  flg_tmp   = (reg_p & FL_C) ? 1:0; \
  UInt16 temp_addr = get_16bit_addr(reg_pc+1); \
  UChar  temp1	   = get_8bit_addr(temp_addr); \
  UChar  temp	   = (temp1<<1)+flg_tmp; \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp1 & 0x80) ? FL_C:0) \
	  | flnz_list[temp]; \
  cycles+=7; \
  put_8bit_addr(temp_addr, temp); \
  reg_pc+=3; \
  } while (0)


# define  rol_absx() do { \
  UChar  flg_tmp   = (reg_p & FL_C) ? 1:0; \
  UInt16 temp_addr = get_16bit_addr(reg_pc+1)+reg_x; \
  UChar  temp1	   = get_8bit_addr(temp_addr); \
  UChar  temp	   = (temp1<<1)+flg_tmp; \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp1 & 0x80) ? FL_C:0) \
	  | flnz_list[temp]; \
  cycles+=7; \
  put_8bit_addr(temp_addr, temp); \
  reg_pc+=3; \
  } while (0)


# define  rol_zp() do { \
  UChar  flg_tmp = (reg_p & FL_C) ? 1:0; \
  UChar  zp_addr = imm_operand(reg_pc+1); \
  UChar  temp1	 = get_8bit_zp(zp_addr); \
  UChar  temp	 = (temp1<<1)+flg_tmp; \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp1 & 0x80) ? FL_C:0) \
	  | flnz_list[temp]; \
  put_8bit_zp(zp_addr, temp); \
  reg_pc+=2; \
  cycles+=6; \
  } while (0)


# define  rol_zpx() do { \
  UChar  flg_tmp = (reg_p & FL_C) ? 1:0; \
  UChar  zp_addr = imm_operand(reg_pc+1)+reg_x; \
  UChar  temp1	 = get_8bit_zp(zp_addr); \
  UChar  temp	 = (temp1<<1)+flg_tmp; \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp1 & 0x80) ? FL_C:0) \
	  | flnz_list[temp]; \
  put_8bit_zp(zp_addr, temp); \
  reg_pc+=2; \
  cycles+=6; \
  } while (0)


# define  ror_a() do { \
  UChar flg_tmp = (reg_p & FL_C) ? 0x80:0; \
  UChar temp	= reg_a; \
 \
  reg_a = (reg_a/2)+flg_tmp; \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp & 0x01) ? FL_C:0) \
	  | flnz_list[reg_a]; \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  ror_abs() do { \
  UChar  flg_tmp   = (reg_p & FL_C) ? 0x80:0; \
  UInt16 temp_addr = get_16bit_addr(reg_pc+1); \
  UChar  temp1	   = get_8bit_addr(temp_addr); \
  UChar  temp	   = (temp1/2)+flg_tmp; \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp1 & 0x01) ? FL_C:0) \
	  | flnz_list[temp]; \
  cycles+=7; \
  put_8bit_addr(temp_addr, temp); \
  reg_pc+=3; \
  } while (0)


# define  ror_absx() do { \
  UChar  flg_tmp   = (reg_p & FL_C) ? 0x80:0; \
  UInt16 temp_addr = get_16bit_addr(reg_pc+1)+reg_x; \
  UChar  temp1	   = get_8bit_addr(temp_addr); \
  UChar  temp	   = (temp1/2)+flg_tmp; \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp1 & 0x01) ? FL_C:0) \
	  | flnz_list[temp]; \
  cycles+=7; \
  put_8bit_addr(temp_addr, temp); \
  reg_pc+=3; \
  } while (0)


# define  ror_zp() do { \
  UChar  flg_tmp = (reg_p & FL_C) ? 0x80:0; \
  UChar  zp_addr = imm_operand(reg_pc+1); \
  UChar  temp1	 = get_8bit_zp(zp_addr); \
  UChar  temp	 = (temp1/2)+flg_tmp; \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp1 & 0x01) ? FL_C:0) \
	  | flnz_list[temp]; \
  put_8bit_zp(zp_addr, temp); \
  reg_pc+=2; \
  cycles+=6; \
  } while (0)


# define  ror_zpx() do { \
  UChar  flg_tmp = (reg_p & FL_C) ? 0x80:0; \
  UChar  zp_addr = imm_operand(reg_pc+1)+reg_x; \
  UChar  temp1	 = get_8bit_zp(zp_addr); \
  UChar  temp	 = (temp1/2)+flg_tmp; \
 \
  reg_p = (reg_p & ~(FL_N|FL_T|FL_Z|FL_C)) \
	  | ((temp1 & 0x01) ? FL_C:0) \
	  | flnz_list[temp]; \
  put_8bit_zp(zp_addr, temp); \
  reg_pc+=2; \
  cycles+=6; \
  } while (0)


# define  rti() do { \
  /* FL_B reset in RTI */ \
  reg_p = pull_8bit() & ~FL_B; \
  reg_pc = pull_16bit(); \
  cycles+=7; \
  } while (0)


# define  rts() do { \
  reg_p &= ~FL_T; \
  reg_pc = pull_16bit()+1; \
  cycles+=7; \
  } while (0)


# define  sax() do { \
  UChar temp = reg_x; \
  reg_p &= ~FL_T; \
  reg_x = reg_a; \
  reg_a = temp; \
  reg_pc++; \
  cycles+=3; \
  } while (0)


# define  say() do { \
  UChar temp = reg_y; \
  reg_p &= ~FL_T; \
  reg_y = reg_a; \
  reg_a = temp; \
  reg_pc++; \
  cycles+=3; \
  } while (0)


# define  sbc_abs() do { \
  sbc(abs_operand(reg_pc+1)); \
  reg_pc+=3; \
  cycles+=5; \
  } while (0)


# define  sbc_absx() do { \
  sbc(absx_operand(reg_pc+1)); \
  reg_pc+=3; \
  cycles+=5; \
  } while (0)


# define  sbc_absy() do { \
  sbc(absy_operand(reg_pc+1)); \
  reg_pc+=3; \
  cycles+=5; \
  } while (0)


# define  sbc_imm() do { \
  sbc(imm_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=2; \
  } while (0)


# define  sbc_zp() do { \
  sbc(zp_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  sbc_zpx() do { \
  sbc(zpx_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  sbc_zpind() do { \
  sbc(zpind_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  sbc_zpindx() do { \
  sbc(zpindx_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  sbc_zpindy() do { \
  sbc(zpindy_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  sec() do { \
  reg_p = (reg_p|FL_C) & ~FL_T; \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  sed() do { \
  reg_p = (reg_p|FL_D) & ~FL_T; \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  sei() do { \
  reg_p = (reg_p|FL_I) & ~FL_T; \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  set() do { \
  reg_p |= FL_T; \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  smb0() do { \
  UChar temp = imm_operand(reg_pc+1); \
  reg_p &= ~FL_T; \
  put_8bit_zp(temp, get_8bit_zp(temp) | 0x01); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  smb1() do { \
  UChar temp = imm_operand(reg_pc+1); \
  reg_p &= ~FL_T; \
  put_8bit_zp(temp, get_8bit_zp(temp) | 0x02); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  smb2() do { \
  UChar temp = imm_operand(reg_pc+1); \
  reg_p &= ~FL_T; \
  put_8bit_zp(temp, get_8bit_zp(temp) | 0x04); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  smb3() do { \
  UChar temp = imm_operand(reg_pc+1); \
  reg_p &= ~FL_T; \
  put_8bit_zp(temp, get_8bit_zp(temp) | 0x08); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  smb4() do { \
  UChar temp = imm_operand(reg_pc+1); \
  reg_p &= ~FL_T; \
  put_8bit_zp(temp, get_8bit_zp(temp) | 0x10); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  smb5() do { \
  UChar temp = imm_operand(reg_pc+1); \
  reg_p &= ~FL_T; \
  put_8bit_zp(temp, get_8bit_zp(temp) | 0x20); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  smb6() do { \
  UChar temp = imm_operand(reg_pc+1); \
  reg_p &= ~FL_T; \
  put_8bit_zp(temp, get_8bit_zp(temp) | 0x40); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  smb7() do { \
  UChar temp = imm_operand(reg_pc+1); \
  reg_p &= ~FL_T; \
  put_8bit_zp(temp, get_8bit_zp(temp) | 0x80); \
  reg_pc+=2; \
  cycles+=7; \
  } while (0)


# define  st0() do { \
  reg_p &= ~FL_T; \
  IO_write(0,imm_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  st1() do { \
  reg_p &= ~FL_T; \
  IO_write(2,imm_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  st2() do { \
  reg_p &= ~FL_T; \
  IO_write(3,imm_operand(reg_pc+1)); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  sta_abs() do { \
  reg_p &= ~FL_T; \
  cycles+=5; \
  put_8bit_addr(get_16bit_addr(reg_pc+1), reg_a); \
  reg_pc+=3; \
  } while (0)


# define  sta_absx() do { \
  reg_p &= ~FL_T; \
  cycles+=5; \
  put_8bit_addr(get_16bit_addr(reg_pc+1)+reg_x, reg_a); \
  reg_pc+=3; \
  } while (0)


# define  sta_absy() do { \
  reg_p &= ~FL_T; \
  cycles+=5; \
  put_8bit_addr(get_16bit_addr(reg_pc+1)+reg_y, reg_a); \
  reg_pc+=3; \
  } while (0)


# define  sta_zp() do { \
  reg_p &= ~FL_T; \
  put_8bit_zp(imm_operand(reg_pc+1), reg_a); \
 \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  sta_zpx() do { \
  reg_p &= ~FL_T; \
  put_8bit_zp(imm_operand(reg_pc+1)+reg_x, reg_a); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  sta_zpind() do { \
  reg_p &= ~FL_T; \
  cycles+=7; \
  put_8bit_addr(get_16bit_zp(imm_operand(reg_pc+1)), reg_a); \
  reg_pc+=2; \
  } while (0)


# define  sta_zpindx() do { \
  reg_p &= ~FL_T; \
  cycles+=7; \
  put_8bit_addr(get_16bit_zp(imm_operand(reg_pc+1)+reg_x), reg_a); \
  reg_pc+=2; \
  } while (0)


# define  sta_zpindy() do { \
  reg_p &= ~FL_T; \
  cycles+=7; \
  put_8bit_addr(get_16bit_zp(imm_operand(reg_pc+1))+reg_y, reg_a); \
  reg_pc+=2; \
  } while (0)


# define  stx_abs() do { \
  reg_p &= ~FL_T; \
  cycles+=5; \
  put_8bit_addr(get_16bit_addr(reg_pc+1), reg_x); \
  reg_pc+=3; \
  } while (0)


# define  stx_zp() do { \
  reg_p &= ~FL_T; \
  put_8bit_zp(imm_operand(reg_pc+1), reg_x); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  stx_zpy() do { \
  reg_p &= ~FL_T; \
  put_8bit_zp(imm_operand(reg_pc+1)+reg_y, reg_x); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  sty_abs() do { \
  reg_p &= ~FL_T; \
  cycles+=5; \
  put_8bit_addr(get_16bit_addr(reg_pc+1), reg_y); \
  reg_pc+=3; \
  } while (0)


# define  sty_zp() do { \
  reg_p &= ~FL_T; \
  put_8bit_zp(imm_operand(reg_pc+1), reg_y); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  sty_zpx() do { \
  reg_p &= ~FL_T; \
  put_8bit_zp(imm_operand(reg_pc+1)+reg_x, reg_y); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  stz_abs() do { \
  reg_p &= ~FL_T; \
  cycles+=5; \
  put_8bit_addr(get_16bit_addr(reg_pc+1), 0); \
  reg_pc+=3; \
  } while (0)


# define  stz_absx() do { \
  reg_p &= ~FL_T; \
  cycles+=5; \
  put_8bit_addr((get_16bit_addr(reg_pc+1)+reg_x), 0); \
  reg_pc+=3; \
  } while (0)


# define  stz_zp() do { \
  reg_p &= ~FL_T; \
  put_8bit_zp(imm_operand(reg_pc+1), 0); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  stz_zpx() do { \
  reg_p &= ~FL_T; \
  put_8bit_zp(imm_operand(reg_pc+1)+reg_x, 0); \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)


# define  sxy() do { \
  UChar temp = reg_y; \
  reg_p &= ~FL_T; \
  reg_y = reg_x; \
  reg_x = temp; \
  reg_pc++; \
  cycles+=3; \
  } while (0)


# define  tai() do { \
  UInt16 from, to, len, alternate; \
 \
  reg_p &= ~FL_T; \
  from = get_16bit_addr(reg_pc+1); \
  to   = get_16bit_addr(reg_pc+3); \
  len  = get_16bit_addr(reg_pc+5); \
  alternate = 0; \
 \
  cycles+=(6 * len) + 17; \
  while (len-- != 0) { \
    put_8bit_addr(to++, get_8bit_addr(from+alternate)); \
    alternate ^= 1; \
  } \
  reg_pc+=7; \
  } while (0)


# define  tam() do { \
  UInt16 i; \
  UChar bitfld = imm_operand(reg_pc+1); \
 \
  for (i = 0; i < 8; i++) { \
    if (bitfld & (1 << i)) { \
      mmr[i] = reg_a; \
      bank_set(i, reg_a); \
    } \
  } \
 \
  reg_p &= ~FL_T; \
  reg_pc+=2; \
  cycles+=5; \
  } while (0)


# define  tax() do { \
  chk_flnz_8bit(reg_x = reg_a); \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  tay() do { \
  chk_flnz_8bit(reg_y = reg_a); \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  tdd() do { \
  UInt16 from, to, len; \
 \
  reg_p &= ~FL_T; \
  from = get_16bit_addr(reg_pc+1); \
  to   = get_16bit_addr(reg_pc+3); \
  len  = get_16bit_addr(reg_pc+5); \
 \
  cycles+=(6 * len) + 17; \
  while (len-- != 0) { \
    put_8bit_addr(to--, get_8bit_addr(from--)); \
  } \
  reg_pc+=7; \
  } while (0)


# define  tia() do { \
  UInt16 from, to, len, alternate; \
 \
  reg_p &= ~FL_T; \
  from = get_16bit_addr(reg_pc+1); \
  to   = get_16bit_addr(reg_pc+3); \
  len  = get_16bit_addr(reg_pc+5); \
  alternate = 0; \
 \
  cycles+=(6 * len) + 17; \
  while (len-- != 0) { \
    put_8bit_addr(to+alternate, get_8bit_addr(from++)); \
    alternate ^= 1; \
  } \
  reg_pc+=7; \
  } while (0)


# define  tii() do { \
  UInt16 from, to, len; \
 \
  reg_p &= ~FL_T; \
  from = get_16bit_addr(reg_pc+1); \
  to   = get_16bit_addr(reg_pc+3); \
  len  = get_16bit_addr(reg_pc+5); \
 \
  cycles+=(6 * len) + 17; \
  while (len-- != 0) { \
    put_8bit_addr(to++, get_8bit_addr(from++)); \
  } \
  reg_pc+=7; \
  } while (0)


# define  tin() do { \
  UInt16 from, to, len; \
 \
  reg_p &= ~FL_T; \
  from = get_16bit_addr(reg_pc+1); \
  to   = get_16bit_addr(reg_pc+3); \
  len  = get_16bit_addr(reg_pc+5); \
 \
  cycles+=(6 * len) + 17; \
  while (len-- != 0) { \
    put_8bit_addr(to, get_8bit_addr(from++)); \
  } \
  reg_pc+=7; \
  } while (0)


# define  tma() do { \
  int i; \
  UChar bitfld = imm_operand(reg_pc+1); \
  for (i = 0; i < 8; i++) { \
    if (bitfld & (1 << i)) { \
      reg_a = mmr[i]; \
    } \
  } \
  reg_p &= ~FL_T; \
  reg_pc+=2; \
  cycles+=4; \
  } while (0)

# define  trb_abs() do { \
  UInt16 abs_addr = get_16bit_addr(reg_pc+1); \
  UChar  temp	  = get_8bit_addr(abs_addr); \
  UChar  temp1	  = (~reg_a) & temp; \
 \
  reg_p = (reg_p & ~(FL_N|FL_V|FL_T|FL_Z)) \
	  | ((temp1&0x80) ? FL_N:0) \
	  | ((temp1&0x40) ? FL_V:0) \
	  | ((temp & reg_a) ? 0:FL_Z); \
  cycles+=7; \
  put_8bit_addr(abs_addr, temp1); \
  reg_pc+=3; \
  } while (0)


# define  trb_zp() do { \
  UChar zp_addr  = imm_operand(reg_pc+1); \
  UChar temp	 = get_8bit_zp(zp_addr); \
  UChar temp1	 = (~reg_a) & temp; \
 \
  reg_p = (reg_p & ~(FL_N|FL_V|FL_T|FL_Z)) \
	  | ((temp1&0x80) ? FL_N:0) \
	  | ((temp1&0x40) ? FL_V:0) \
	  | ((temp & reg_a) ? 0:FL_Z); \
  put_8bit_zp(zp_addr, temp1); \
  reg_pc+=2; \
  cycles+=6; \
  } while (0)


# define  tsb_abs() do { \
  UInt16 abs_addr = get_16bit_addr(reg_pc+1); \
  UChar  temp	  = get_8bit_addr(abs_addr); \
  UChar  temp1	  = reg_a | temp; \
 \
  reg_p = (reg_p & ~(FL_N|FL_V|FL_T|FL_Z)) \
	  | ((temp1&0x80) ? FL_N:0) \
	  | ((temp1&0x40) ? FL_V:0) \
	  | ((temp & reg_a) ? 0:FL_Z); \
  cycles+=7; \
  put_8bit_addr(abs_addr, temp1); \
  reg_pc+=3; \
  } while (0)


# define  tsb_zp() do { \
  UChar zp_addr  = imm_operand(reg_pc+1); \
  UChar temp	 = get_8bit_zp(zp_addr); \
  UChar temp1	 = reg_a | temp; \
 \
  reg_p = (reg_p & ~(FL_N|FL_V|FL_T|FL_Z)) \
	  | ((temp1&0x80) ? FL_N:0) \
	  | ((temp1&0x40) ? FL_V:0) \
	  | ((temp & reg_a) ? 0:FL_Z); \
  put_8bit_zp(zp_addr, temp1); \
  reg_pc+=2; \
  cycles+=6; \
  } while (0)


# define  tstins_abs() do { \
  UChar  imm	  = imm_operand(reg_pc+1); \
  UChar  temp	  = abs_operand(reg_pc+2); \
 \
  reg_p = (reg_p & ~(FL_N|FL_V|FL_T|FL_Z)) \
	  | ((temp&0x80) ? FL_N:0) \
	  | ((temp&0x40) ? FL_V:0) \
	  | ((temp&imm)  ? 0:FL_Z); \
  cycles+=8; \
  reg_pc+=4; \
  } while (0)


# define  tstins_absx() do { \
  UChar  imm	  = imm_operand(reg_pc+1); \
  UChar  temp	  = absx_operand(reg_pc+2); \
 \
  reg_p = (reg_p & ~(FL_N|FL_V|FL_T|FL_Z)) \
	  | ((temp&0x80) ? FL_N:0) \
	  | ((temp&0x40) ? FL_V:0) \
	  | ((temp&imm)  ? 0:FL_Z); \
  cycles+=8; \
  reg_pc+=4; \
  } while (0)


# define  tstins_zp() do { \
  UChar imm	= imm_operand(reg_pc+1); \
  UChar temp	= zp_operand(reg_pc+2); \
 \
  reg_p = (reg_p & ~(FL_N|FL_V|FL_T|FL_Z)) \
	  | ((temp&0x80) ? FL_N:0) \
	  | ((temp&0x40) ? FL_V:0) \
	  | ((temp&imm)  ? 0:FL_Z); \
  cycles+=7; \
  reg_pc+=3; \
  } while (0)


# define  tstins_zpx() do { \
  UChar imm	= imm_operand(reg_pc+1); \
  UChar temp	= zpx_operand(reg_pc+2); \
 \
  reg_p = (reg_p & ~(FL_N|FL_V|FL_T|FL_Z)) \
	  | ((temp&0x80) ? FL_N:0) \
	  | ((temp&0x40) ? FL_V:0) \
	  | ((temp&imm)  ? 0:FL_Z); \
  cycles+=7; \
  reg_pc+=3; \
  } while (0)

# define  tsx() do { \
  chk_flnz_8bit(reg_x = reg_s); \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  txa() do { \
  chk_flnz_8bit(reg_a = reg_x); \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  txs() do { \
  reg_p &= ~FL_T; \
  reg_s = reg_x; \
  reg_pc++; \
  cycles+=2; \
  } while (0)


# define  tya() do { \
  chk_flnz_8bit(reg_a = reg_y); \
  reg_pc++; \
  cycles+=2; \
  } while (0)


// perform machine operations for IRQ2:

static inline int int_irq2 (void) 
{
  if ((io.irq_mask & FL_IRQ2) != 0) {   // interrupt disabled
     return 0;
  }
//if ((irq_register & FL_IRQ2) == 0) {	 // interrupt disabled ?
//   return 0;
//}
  cycles+=7;
  push_16bit(reg_pc);
  push_8bit(reg_p);
  reg_p = (reg_p & ~FL_D) | FL_I;
  reg_pc = get_16bit_addr(0xFFF6);
  return 0;
}

// perform machine operations for IRQ1 (video interrupt):

static inline int int_irq1 (void) 
{
  if ((io.irq_mask & FL_IRQ1) != 0) {   // interrupt disabled
     return 0;
  }
  cycles+=7;
  push_16bit(reg_pc);
  push_8bit(reg_p);
  reg_p = (reg_p & ~FL_D) | FL_I;
  reg_pc = get_16bit_addr(0xFFF8);
  return 0;
}

// perform machine operations for timer interrupt:

static inline int int_tiq(void) {
  if ((io.irq_mask & FL_TIQ) != 0) {    // interrupt disabled
     return 0;
  }
//if ((irq_register & FL_TIQ) == 0) {	// interrupt disabled ?
//   return 0;
//}
  cycles+=7;
  push_16bit(reg_pc);
  push_8bit(reg_p);
  reg_p = (reg_p & ~FL_D) | FL_I;
  reg_pc = get_16bit_addr(0xFFFA);
  return 0;
}

