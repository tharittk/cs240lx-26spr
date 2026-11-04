#include "rpi-asm.h"

@ trampoline used to:
@   - save all callee saved registers to stack.
@   - call ck_gc_fn(sp)
@   - return
@
@  note: also have to save/restore lr
MK_FN(ck_gc)
	push {r4-r11, lr}
	
	mov r0, sp
	bl ck_gc_fn
	
	pop {r4-r11, lr}
	bx lr

@ similar trampoline as above, but will call
@ <ck_find_leaks_fn>.  note: sp is in a different
@ register
MK_FN(ck_find_leaks)
	push {r4-r11, lr}
	
	mov r1, sp
	bl ck_find_leaks_fn
	
	pop {r4-r11, lr}
	bx lr
