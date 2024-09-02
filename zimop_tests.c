#include <rvh_test.h>

static inline void zimop_r(uintptr_t value){
    asm volatile(
        ".insn r 0x73, 0x4, 0x40, %0, x0, x28\n\t"
        : "=r"(value));
}

static inline void zimop_rr(uintptr_t value){
    asm volatile(
        ".insn r 0x73, 0x4, 0x41, %0, x0, x0\n\t"
        : "=r"(value));
}

bool zimop_tests(){

    TEST_START();

    for (int i = 0; i < 32; i++){
        // change the vd of the instruction
        zimop_r(i);
        zimop_rr(i);
    } 

    TEST_END();
}
