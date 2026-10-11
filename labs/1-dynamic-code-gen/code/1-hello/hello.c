// dynamically generate code to call a routine.  we
// do this in two steps to make it easier to debug.
//  - first, generate a call to a predefined functions 
//    that take no arguments (hello_before and hello_after).
//  - when that works, generate a call to a routine that takes
//    a 32-bit argument.  the general way that arm handles this:
//       - write the 32-bit value in the code array
//         *after* all the code you generate
//       - emit an ldr instruction using the pc to load
//         it.
#include "rpi.h"
#include "rpi-interrupts.h"

// we use this to catch if you jump off by one
static void guard1(void) { asm volatile ("bkpt"); }

static void hello1(void) { 
    printk("hello world 1\n");
}

// we use this to catch if you jump off by one
static void guard2(void) { asm volatile ("bkpt"); }

void hello2(void) { 
    printk("hello world 2\n");
}

// we use this to catch if you jump off by one
static void guard3(void) { asm volatile ("bkpt"); }

// the routines to implement.
static inline uint32_t armv6_push(int reg) {
	// (tt):	e52de004	push    {lr}	; (str lr, [sp, #-4]!)
    assert(reg<16);
    uint32_t code = 0xe52de004;
    // d = 13 (sp), e = 14 (lr)
    // we then replace e with reg
    code &= ~0xf000;
    code |= (reg << 12);
    return code;
}
static inline uint32_t armv6_pop(int reg) {
    // (tt):	e8bd8010        pop     {r4, pc}
    assert(reg<16);
	// 8010 is a register bit mask (1000 <0> 0001 <0>)
	// which means reg 15 and reg 4 are selected.
	uint32_t code = 0xe8bd8010;
	code &= ~0xffff;
	code |= (1 << reg);
	return code;
}

// pc = where the instruction will be put.  this is 
// needed so that you can compute the offset from <pc>
// to <addr> which is what gets put in <bl>
static inline uint32_t armv6_bl(uint32_t bl_pc, uint32_t target) {
	/* (tt): 8058: eb000020 bl 80e0 <printk>
	 * e is AL (cond)
	 * b is 1011 (L = 1)
	 * signed immed 24 = 000020 while target_addr = 80e0
	 * from armv6,  target_addr = 
	 *		- sign_extend(0x20) -- still 0x20
	 *		- shift left 2 bit -- 0x80
	 *		- add this to pc (bl location + 8);
	 *			-- 0x8058 + 0x8 = 0x8060; 0x8060+0x80 = 0x80e0
	 *		sanity checked !
	 */

    uint32_t code = 0;

	/* Now we want to reverse that (goes from target to sign imm 24)
    * bit 23:0 is to construct signed immed 24 from target, follow A4.1.5 
    * subtract (pc+8), right shift 2, remove sign extension .
    */
    uint32_t imm;
    imm = target - (bl_pc + 8);
    imm = (imm >> 2);

    code = (0xE << 28) | (0b1011 << 24) | (0xffffff & imm);
    return code;
}

static inline uint32_t armv6_bx(uint32_t reg) {
    assert(reg<16);
	// (tt): <printk>
	// e12fff1e; bx lr (lr = 14 = e)
	uint32_t code = 0xe12fff1e;
	code &= ~0b0000;
	code |= reg;

    return code;
}

static inline uint32_t 
armv6_ldr(uint32_t dst_reg, uint32_t src_reg, uint32_t off) {
    assert(dst_reg<16);
    assert(src_reg<16);
    todo("return the machine code to: ldr <dst>, [<src>+#<off>]\n");
    uint32_t code = 0;
}

// generate a dynamic call to hello() 
void jit_hello(void *fn, void * arg) {
    static uint32_t code[8];
    

    // a few of the registers
    enum {
        lr = 14,
        pc = 15,
        sp = 13,
        r0 = 0,
    };

    uint32_t addr =(uint32_t)fn;
    uint32_t n = 0;

    // generate a trampoline to call <fn>.
    //   1. we need to save and restore <lr> since the 
    //      call (bl) will trash it.
    //   2. need to make sure you sign extend <addr> in bl
    //      correctly so that it works with a negative offset!
    //  (tt): to be clear, this current lr we are saving
    //  is the returned address of the caller of this 
    //  (think about nested call).
    code[n++] = armv6_push(lr);

    // to see what value the pc register has when you read it
    // you can look at <prelab-code-pi/4-derive-pc-reg.c>
    // or also read the manual :)
    if(arg) {
        todo("extend this code to handle a 32-bit argument!\n");
    }

    uint32_t src = (uint32_t)&code[n];
    code[n++] = armv6_bl(src, addr);
    code[n++] = armv6_pop(lr);
    code[n++] = armv6_bx(lr);

    printk("emitted code at %x to call routine (%x):\n", code, addr);
    for(int i = 0; i < 4; i++) 
        printk("code[%d]=0x%x\n", i, code[i]);

    void (*fp)(void) = (typeof(fp))code;
    printk("about to call: %x\n", fp);
    printk("--------------------------------------\n");
    fp();
    printk("--------------------------------------\n");
}

void notmain(void) {
    // so we can catch some exceptions.
    interrupt_init();

    // step 1: generate calls that take no arguments.
    jit_hello(hello1, 0);
    jit_hello(hello2, 0);
    // step 2: generate calls that take a single argument
    jit_hello(printk, "hello world\n");
    jit_hello(printk, "hello world2\n");
}
