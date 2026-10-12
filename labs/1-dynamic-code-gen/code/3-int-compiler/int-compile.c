#include "rpi.h"

#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
#include "cycle-util.h"

typedef void (*int_fp)(void);

static volatile unsigned cnt = 0;

// fake little "interrupt" handlers: useful just for measurement.
void int_0() { cnt++; }
void int_1() { cnt++; }
void int_2() { cnt++; }
void int_3() { cnt++; }
void int_4() { cnt++; }
void int_5() { cnt++; }
void int_6() { cnt++; }
void int_7() { cnt++; }

static int_fp intv[] = {
        int_0,
        int_1,
        int_2,
        int_3,
        int_4,
        int_5,
        int_6,
        int_7
};

void generic_call_int(int_fp *intv, unsigned n) { 
    for(unsigned i = 0; i < n; i++)
        intv[i]();
}

// you will generate this dynamically.
void specialized_call_int(void) {
		int_0();
		int_1();
		int_2();
		int_3();
		int_4();
		int_5();
		int_6();
		int_7();
}
void specialized_call_int_helper(void) {
	uint32_t bl_pc, target, imm;
	int n_int = sizeof(intv) / sizeof(intv[0]);
	int n = 0;
	uint32_t *code = (uint32_t *)specialized_call_int;

	/* push {lr} */
	*code = 0xe52de004;
	code++;


	for (int i = 0; i < n_int - 1; i++) {
		/* bl int_i */
		bl_pc = (uint32_t)&code[n];
		target = (uint32_t)intv[i];
		imm = target - (bl_pc + 8);
		imm = (imm >> 2);
		*code= (0xE << 28) | (0xb) << 24 | (0xffffff & imm);
		code++;
	}

	/* pop lr so that last interrupt bx lr goes back to caller */
	*code = 0xe8bd4000;
	code++;

	/* bl int[n_int - 1] */
	bl_pc = (uint32_t)code;
	target = (uint32_t)intv[n_int - 1];
	imm = target - (bl_pc + 8);
	imm = (imm >> 2);
	*code = (0xE << 28) | (0xb) << 24 | (0xffffff & imm);
	code++;
}

/* only one instruction needed, the caller in notmain does bl thread_fn;
so it puts the return address in lr, any b call will not trash that.*/
static uint32_t thread_code[1];
void thread_code_init(void) {
	uint32_t b_pc, target, imm;
	int n_int = sizeof(intv) / sizeof(intv[0]);

	/* pop lr so that last interrupt bx lr goes back to caller */
	//thread_code[0] = 0xe8bd4000;

	/* b int_0 */
	b_pc = (uint32_t)thread_code;
	target = (uint32_t)intv[0];
	imm = target - (b_pc + 8);
	imm = (imm >> 2);
	thread_code[0] = (0xE << 28) | (0xa) << 24 | (0xffffff & imm);

	/* call b int_{i+1} at the end of each int {i}. 
	 * The int_n still does bx lr which wll back to
	 * caller */
	for (int i = 0; i < n_int - 1; i++) {
		/* b int_i */
		uint32_t int_code = (uint32_t)intv[i];
		for (uint32_t addr = int_code;; addr += 4) {
			if ( *(uint32_t*)addr == 0xe12fff1e) { /* bx lr */
					b_pc = addr;
					target = (uint32_t)intv[i+1];
					imm = target - (b_pc + 8);
					imm = (imm >> 2);
					/* b int_i+1 */
					*(uint32_t*)addr = (0xE << 28) | (0xa) << 24 | (0xffffff & imm);
					break;
			}
		}
	}
	/* last interrupt is kept bx lr. The lr has return address by the caller */
}

void notmain(void) {
    cycle_cnt_init();

    unsigned n = NELEM(intv);

    // try with and without cache: but if you modify the routines to do 
    // jump-threadig, must either:
    //  1. generate code when cache is off.
    //  2. invalidate cache before use.
    // enable_cache();

    cnt = 0;
    TIME_CYC_PRINT10("cost of generic-int calling",  generic_call_int(intv,n));
    demand(cnt == n*10, "cnt=%d, expected=%d\n", cnt, n*10);

    // rewrite to generate specialized caller dynamically.
    cnt = 0;
	
    TIME_CYC_PRINT10("cost of specialized int calling", specialized_call_int() );
    demand(cnt == n*10, "cnt=%d, expected=%d\n", cnt, n*10);

	cnt = 0;
	thread_code_init();
	void (*thread_fn)(void) = (typeof(thread_fn))thread_code;
    TIME_CYC_PRINT10("cost of threaded specialized int calling", thread_fn());
    demand(cnt == n*10, "cnt=%d, expected=%d\n", cnt, n*10);

    clean_reboot();
}
