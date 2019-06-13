/****************************************************************************
 h6280.h
 Function protoypes for simulated execution routines
 ****************************************************************************/

#ifndef H6280_H_
#define H6280_H_

#include "cleantyp.h"

/********************************************/
/* function parameters:                     */
/* --------------------                     */
/* - address (16-bit unsigned),             */
/* - pointer to buffer @ program counter    */
/********************************************/

extern void exe_go(void);


#define INT_NONE        0            /* No interrupt required      */
#define INT_IRQ         1            /* Standard IRQ interrupt     */
#define INT_NMI         2            /* Non-maskable interrupt     */
#define INT_QUIT        3            /* Exit the emulation         */
#define	INT_TIMER       4
#define	INT_IRQ2        8

#define	VEC_RESET	0xFFFE
#define	VEC_NMI		0xFFFC
#define	VEC_TIMER	0xFFFA
#define	VEC_IRQ		0xFFF8
#define	VEC_IRQ2	0xFFF6
#define	VEC_BRK		0xFFF6

extern UChar flnz_list[256];

# if 0 //LUDO:
inline UChar imm_operand(UInt16 addr);
inline UChar get_8bit_zp(UChar zp_addr);
inline UInt16 get_16bit_zp(UChar zp_addr);
inline void put_8bit_zp(UChar zp_addr, UChar byte);

inline UChar get_8bit_addr(UInt16 addr);
inline void put_8bit_addr(UInt16 addr, UChar byte);
inline UInt16 get_16bit_addr(UInt16 addr);
# endif

#endif
