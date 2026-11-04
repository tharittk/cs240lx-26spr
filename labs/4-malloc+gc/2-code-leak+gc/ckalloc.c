// implement a simple ckalloc/free that adds ckhdr_t to the 
// allocation.
#include "rpi.h"
#include "ckalloc.h"
#include "kr-malloc.h"

unsigned ck_verbose_p = 0;

// should hold all allocated blocks
static hdr_t *alloc_list; 

// returns pointer to the first allocated header block.
hdr_t *ck_first_alloc(void) {
    return alloc_list;
}

// return header associated with <ptr> if one exists.
hdr_t *ck_ptr_is_alloced(void *ptr) {
    for(hdr_t *h = ck_first_alloc(); h; h = ck_next_hdr(h))
        if(ck_ptr_in_block(h,ptr)) 
            return h;
    return 0;
}


/***********************************************************************
 * implement the rest
 */

static void list_remove(hdr_t **l, hdr_t *h)
{
		assert(l);
		hdr_t *prev = *l;
		if (prev == h) {
				*l = h->next;
				return;
		}

		hdr_t *p;
		while ((p = ck_next_hdr(prev))) {
				if (p == h) {
						prev->next = p->next;
						return;
				}
				prev = p;
		}
		panic ("did not find %p in list \n", h);
}

static void list_add(hdr_t **l, hdr_t *h)
{
		if (!(*l)) {
				*l = h;
		} else {
				h->next = *l;
				*l = h;
		}
}

// is <ptr> inside <h>'s data block?
unsigned ck_ptr_in_block(hdr_t *h, void *ptr) {
    if(h->state != ALLOCED) {
        panic("should only have allocated blocks: sdtate=%d\n", h->state);
        return 0;
    }

	unsigned addr = (unsigned) ptr;
	unsigned start = (unsigned) ck_data_start(h);
	unsigned end = (unsigned) ck_data_end(h);
	return (addr >= start) && (addr < end);
}


// free a block allocated with <ckalloc>
void (ckfree)(void *addr, src_loc_t l) {
    hdr_t *h = ck_ptr_is_alloced(addr);
    if(!h)
        loc_panic(l, "freeing bogus pointer: %p\n", addr);

    // allocated block starts right after the header.

    void *blk_start = ck_data_start(h);
    if(blk_start != addr)
        loc_panic(l, "not freeing using start pointer: have %p, need %p\n",
            addr, blk_start);

    if(h->state != ALLOCED)
        loc_panic(l, "freeing unallocated memory: state=%d\n", h->state);
    if(ck_verbose_p)
        loc_debug(l, "freeing %p\n", addr);

    assert(ck_ptr_is_alloced(addr));
    h->state = FREED;

    // just remove from the allocated list.
	list_remove(&alloc_list, h);

    kr_free(h);
}


// interpose on kr_malloc allocations and
//  1. allocate enough space for a header and fill it in.
//  2. add the allocated block to  the allocated list.
void *(ckalloc)(uint32_t nbytes, src_loc_t l) {
    static unsigned block_id=1;

    hdr_t *h = kr_malloc(nbytes + sizeof *h);

    memset(h, 0, sizeof *h);
    h->nbytes_alloc = nbytes;
    h->state = ALLOCED;
    h->alloc_loc = l;
    h->block_id = block_id++;

    // set addr;
    void *addr = 0;

	list_add(&alloc_list, h);
	addr = ck_data_start(h);

    assert(ck_ptr_is_alloced(addr));
    if(ck_verbose_p)
        loc_debug(l, "successful alloc of %p\n", addr);
    return addr;
}
