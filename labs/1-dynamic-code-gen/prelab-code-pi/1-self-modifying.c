// simple example of self-modifying code.
#include "rpi.h"

/*
   from the list file:
    00008038 <ret_10>:
        8038:   e3a0000a    mov r0, #10
        803c:   e12fff1e    bx  lr
 */
int ret_10(void) { return 10; }

/* 
   from the list file:
    00008040 <ret_11>:
        8040:   e3a0000b    mov r0, #11
        8044:   e12fff1e    bx  lr
 */
int ret_11(void) { return 11; }

/* e2600000 */
int ret_neg(int a) {return -a;}

void notmain() { 
    // Q: if we enable the icache, what is going on?
    // A: instruction cache being enabled means the
    // processor knows that it is fetching the instruction
    // at the memory address it is known to be fetched
    // recently. So it skips the memory access.
    // That means when we do code[0] = sth else (#10),
    // we will not see its effect and still return #11.
    //enable_cache();

    // generate the code to return 11
    //
    // Q: why might expect this needs to be volatile?
    // [doesn't seem to w/ our compiler]
    // A: because we may change the content of this
    // i.e., when reassign code[0] to different byte 
    // pattern. Otherwise, the compiler may optimize
    // away. But I think this is ok because code[0] = #10
    // marks that the variabled is changed and compiler cannot
    // optimized away across the boundary. This is different
    // than device memory read where the content must be refreshed
    // so that it is in sync with the device.
	unsigned code[3];

    // from above:
    //  00008040 <ret_11>:
    //      8040:   e3a0000b    mov r0, #11
    //      8044:   e12fff1e    bx  lr
    code[0] = 0xe3a0000b;       // mov r0, #11
    code[1] = 0xe12fff1e;       // bx lr

    // cast array address to function pointer.
	int (*fp)(void) = (int (*)(void))code;

    // call it.
    unsigned x = fp();
	printk("fp() = %d [should be 11]\n", x);
    assert(x == 11);

    // change the code to return 10
    printk("about to make code return 10\n");
    asm volatile(".align 5");
    code[0] = 0xe3a0000a; // mov r0, #10

    // Q: experiment w/ deleting these: what is going on?
    // Q: how many can we delete? why?
    // A: I suspect that it is about the superscalar out-of-order
    // execution model where the instruction is issued out-of-order
    // and speculatively load. That means the x=fp() may load the old
    // definition of code[0] before the memory write takes an effect.
    //
    // Nope. close. It is the instruction prefetching (pipelining).
    asm volatile ("nop"); // 0
    asm volatile ("nop"); // 1
    asm volatile ("nop"); // 2
    asm volatile ("nop"); // 3
    asm volatile ("nop"); // 4
    asm volatile ("nop"); // 5
    asm volatile ("nop"); // 6
    asm volatile ("nop"); // 7

    x = fp();
    printk("fp() = %d [should be 10]\n", x);
    assert(x == 10);

    enum { N = 255 };
    printk("about to try [0,%d)\n", N);
    for(unsigned u = 0; u < N; u++) {
        // clear and set the bits for immediate
        //
        // Q: how do you set them to return a negative value?
        code[0] = (code[0] & ~0xff) | u;
	// A: do 2's complement: invert the bit and add 1
	// well, the does not work. we can add neg instruction
	// or do mvn

        // Q: if delete this?  how different from nops?
	// A: this makes the next instruction line up with
	// the cache-line boundry / prefetch block.
	// removing it then we might using i-cache and thus test fails.
        asm volatile(".align 5");
        assert(fp() == u);

	// test negative
	// Doing this cause the above test to fail !
	// (the positive test relies on code[])
	//code[2] = code[1];
	//code[1] = 0xe2600000;
        //asm volatile(".align 5");
        //assert(fp() == -u);
    }
    printk("passed %d tests!\n", N);
}
