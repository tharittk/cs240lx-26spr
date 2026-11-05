# Part 2: garbage collection
#PROGS := tests/gc-test9-gc-interior.c
#PROGS := tests/gc-test8-gc-reachable.c
#PROGS := tests/gc-test7-gc-global.c
#PROGS := tests/gc-test6-no-leak.c
#PROGS := tests/gc-test5-gc.c
#PROGS := tests/gc-test4-no-leak.c  
#PROGS := tests/gc-test2-no-leak.c  
#PROGS := tests/gc-test1-no-leak.c  
PROGS := $(wildcard tests/gc-test[0-9]*.c)

# Part 1: simple checking allocation
#PROGS := tests/ck-test1-ok.c
#PROGS := tests/ck-test2-ok.c
#PROGS := tests/ck-test3-ok.c
PROGS := $(wildcard tests/ck-test[0-9]*.c)

# all to start
#PROGS += $(wildcard tests/gc-test[0-9]*.c)

# you should  comment out all staff objs and replace with your code.

COMMON_SRC = ../1-malloc/kr-malloc.c
#STAFF_OBJS += staff-kr-malloc.o

COMMON_SRC += ckalloc.c
#STAFF_OBJS += staff-ckalloc.o

COMMON_SRC += ck-gc.c
#STAFF_OBJS += staff-ck-gc.o

COMMON_SRC += gc-asm.S
#STAFF_OBJS += staff-gc-asm.o

# if you get issues, try swapping this.
OPT_LEVEL = -O0
# OPT_LEVEL = -O2

CFLAGS += -fno-inline
CFLAGS += -fno-inline-functions
CFLAGS += -fno-inline-small-functions
CFLAGS += -fno-default-inline

RUN=0
EXCLUDE ?= grep -v simple_boot
include $(CS240LX_2026_PATH)/libpi/mk/Makefile.robust-v3

checkall:
	make clean
	@echo "Checking optimization level -O0"
	make OPT_LEVEL="-O0" check 
	make clean
	@echo "Checking optimization level -O1"
	make OPT_LEVEL="-O1" check 
	make clean
	@echo "Checking optimization level -O2"
	make OPT_LEVEL="-O2" check 
	make clean
	@echo "Checking optimization level -Ofast"
	make OPT_LEVEL="-Ofast" check 

