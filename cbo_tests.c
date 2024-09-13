#include <rvh_test.h>
#include <page_tables.h> 

bool cbo_test() {

    TEST_START();
    uintptr_t vaddr_f = vs_page_base(SWITCH1);
    uint64_t value = 0xdeadbeef;

    //////////////////////////////////////////////////////////////////////

    CSRW(CSR_MENVCFG, 0x0);
    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    CBOCLEAN();
    TEST_ASSERT("U-mode cbo.clean causes illegal instruction exception when menvcfg.cbcfe=0",
        excpt.triggered == true &&  
        excpt.cause == CAUSE_ILI
    );   

    //////////////////////////////////////////////////////////////////////

    
    TEST_END();
}