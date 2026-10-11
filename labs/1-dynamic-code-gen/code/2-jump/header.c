// print out the string in the header.
#include "rpi.h"

// hardcoded in linker script
extern char __hello_string__[];

void notmain(void) {

    // look in:
    // - <libpi/memmap> for an example of how to define a symbol.
    // - <libpi/include/memmap.h> for an example of how to reference the
    //   symbol from C code.
    // set this to the symbol.
    const char *header_string = __hello_string__;

    assert(header_string);
    printk("<%s>\n", header_string);
    printk("success!\n");
}
