#ifndef __ARMV6_ENCODINGS_H__
#define __ARMV6_ENCODINGS_H__

enum {
    armv6_lr = 14,
    armv6_pc = 15,
    armv6_sp = 13,
    armv6_r0 = 0,

    op_mov = 0b1101,
    op_mvn = 0b1111,
    op_orr = 0b1100,
    op_mult = 0b0000,

    cond_always = 0b1110
};

// trivial wrapper for register values so type-checking works better.
typedef struct {
    uint8_t reg;
} reg_t;
static inline reg_t reg_mk(unsigned r) {
    if(r >= 16)
        panic("illegal reg %d\n", r);
    return (reg_t){ .reg = r };
}

// how do you add a ngative? do a sub?
static inline uint32_t armv6_mov(reg_t rd, reg_t rn) {
    // could return an error.  could get rid of checks.
    //         I        opcode        | rd
    return cond_always << 28
        | op_mov << 21 
        | rd.reg << 12 
        | rn.reg
        ;
}

static inline uint32_t armv6_orr(reg_t rd, reg_t rn) {
    return cond_always << 28
        | op_orr << 21 
        | rd.reg << 12 
        | rn.reg
        ;
}

static inline uint32_t armv6_mvn(reg_t rd, reg_t rn) {
    return cond_always << 28
        | op_mvn << 21 
        | rd.reg << 12 
        | rn.reg
        ;
}


static inline uint32_t 
armv6_mov_imm8_rot4(reg_t rd, uint32_t imm8, unsigned rot4) {
    if(imm8>>8)
        panic("immediate %d does not fit in 8 bits!\n", imm8);
    if(rot4 % 2)
        panic("rotation %d must be divisible by 2!\n", rot4);
    rot4 /= 2;
    if(rot4>>4)
        panic("rotation %d does not fit in 4 bits!\n", rot4);

	reg_t r0 = reg_mk(armv6_r0);
	uint32_t code  = armv6_mov(rd, r0);
	code |= (1 << 25);
	code |= ((0xf & rot4) << 8);
	code |= (0xff & imm8);
	return code;
}
static inline uint32_t 
armv6_mov_imm8(reg_t rd, uint32_t imm8) {
    return armv6_mov_imm8_rot4(rd,imm8,0);
}

static inline uint32_t 
armv6_mvn_imm8(reg_t rd, uint32_t imm8) {
	reg_t r0 = reg_mk(armv6_r0);
	uint32_t code = armv6_mvn(rd, r0);
	code |= (1 << 25);
	code |= (0xff & imm8);
	return code;
}

static inline uint32_t 
armv6_bx(reg_t rd) {
		return (uint32_t)((0xe12fff1 << 4) | (0xf & rd.reg));
}

// use 8-bit immediate imm8, with a 4-bit rotation.
static inline uint32_t 
armv6_orr_imm8_rot4(reg_t rd, reg_t rn, unsigned imm8, unsigned rot4) {
    if(imm8>>8)
        panic("immediate %d does not fit in 8 bits!\n", imm8);
    if(rot4 % 2)
        panic("rotation %d must be divisible by 2!\n", rot4);
    rot4 /= 2;
    if(rot4>>4)
        panic("rotation %d does not fit in 4 bits!\n", rot4);

    uint32_t code = (cond_always << 28) | (op_orr << 21)
        | (rn.reg << 16) | (rd.reg << 12);
	code |= (1 << 25);
	code |= ((0xf & rot4) << 8);
	code |= (0xff & imm8);
	return code;
}

static inline uint32_t 
armv6_orr_imm8(reg_t rd, reg_t rn, unsigned imm8) {
    if(imm8>>8)
        panic("immediate %d does not fit in 8 bits!\n", imm8);
    return armv6_orr_imm8_rot4(rd, rn, imm8, 0);
}

// a4-80
static inline uint32_t 
armv6_mult(reg_t rd, reg_t rm, reg_t rs) {
	uint32_t code = 0xe00 << 20;
	code |= (rd.reg << 16);
	code |= (rs.reg << 8);
	code |= (0x9 << 4);
	code |= (rm.reg);
	return code;
}


// load a word from memory[offset]
// ldr rd, [rn,#offset]
static inline uint32_t 
armv6_ldr_off12(reg_t rd, reg_t rn, int offset) {
	uint32_t u = 1; // 1 = add offset, 0 = substract
	if (offset < 0) {
			u = 0;
			/* caller calculate offset from code. so they may feed negative */
			offset = -offset;
	}
	if (offset & ~0xFFF)
		panic("offset %d is out-of-range: limited to 12 immediates\n", offset);

	uint32_t ldr = (0xe << 28) | (1 << 26) | (1 << 24) | (u << 23) | \
				   (1 << 20) | (rn.reg << 16) | (rd.reg << 12) | \
				   (offset & 0xFFF);
	return ldr;
}

/**********************************************************************
 * synthetic instructions.
 * 
 * these can result in multiple instructions generated so we have to 
 * pass in a location to store them into.
 */

static inline uint32_t *
armv6_load_imm32(uint32_t *code, reg_t rd, uint32_t imm32) {

	//reg_t r0 = reg_mk(armv6_r0);
	code[0] = armv6_mov_imm8(rd, 0);
	code[1] = armv6_orr_imm8_rot4(rd, rd, imm32 & 0xff, 0);
	/* right rotation wraps around, the second byte need rotate 24=12x1*/
	code[2] = armv6_orr_imm8_rot4(rd, rd, (imm32 >> 8) & 0xff, 24);
	code[3] = armv6_orr_imm8_rot4(rd, rd, (imm32 >> 16) & 0xff, 16);
	code[4] = armv6_orr_imm8_rot4(rd, rd, (imm32 >> 24) & 0xff, 8);
	//code[4] = armv6_mov(rd, r0);

    return code + 5;
}

// MLA (Multiply Accumulate) multiplies two signed or unsigned 32-bit
// values, and adds a third 32-bit value.
//
// a4-66: multiply accumulate.
//      rd = rm * rs + rn.
static inline uint32_t
armv6_mla(reg_t rd, reg_t rm, reg_t rs, reg_t rn) {    
	uint32_t code = (0xe << 28) | (1 << 21) | (rd.reg << 16) | (rn.reg << 12)
			| (rs.reg << 8) | (0x9) << 4 | rm.reg;
	return code;
}

#endif
