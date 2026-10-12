#include "rpi-asm.h"

.globl put32_inline
.globl PUT32_inline
put32_inline:
PUT32_inline:
    @ put32_inline will simply return to caller since we
    @ we pass lr in r2  along with the original arguments.

	mov r2, lr
	b PUT32_inline_helper

	@ no need for bx lr (return) because PUT32 returns void
@ get32(addr)
@
@ c-code equivalent: 
@   - get32(addr) { return *(uint32_t*)addr; }
@
@ load value of <addr> passed in first argument register (r0).
@
.globl get32_inline
.globl GET32_inline
get32_inline:
GET32_inline:

    @ we pass lr in r1 along with original argument in r0.
    @ get32_inline will simply return to the caller
    mov r1, lr
    b GET32_inline_helper

    ldr r0,[r0]     @ load address held in r0 into r0
    bx lr           @ return
