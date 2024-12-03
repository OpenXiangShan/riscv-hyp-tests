#include <rvh_test.h>
#include <page_tables.h>

extern struct {
    uint64_t vs;
    uint64_t h;
} test_page_perm_table[TEST_PAGE_MAX];
pte_t hspt_hu[3][PAGE_SIZE/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE)));

void hspt_hu_init(){
    uintptr_t addr;

    addr = 0x00000000;
    for(int i = 0; i < 4; i++){
        hspt_hu[0][i] =
            PTE_V | PTE_U | PTE_AD | PTE_RWX | (addr >> 2);
        addr +=  SUPERPAGE_SIZE(0);
    } // 1 level page, huge page, each page is 1G

    hspt_hu[0][4] =
        PTE_V | (((uintptr_t)&hspt_hu[1][0]) >> 2);
    hspt_hu[1][0] =
        PTE_V | (((uintptr_t)&hspt_hu[2][0]) >> 2);


    addr = TEST_PPAGE_BASE;
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        hspt_hu[2][i] = (addr >> 2) | PTE_AD |
            test_page_perm_table[i].vs;
        addr += PAGE_SIZE;
    }


    if(curr_priv == PRIV_M){
        uintptr_t satp = (((uintptr_t)hspt_hu) >> 12) | (0x8ULL << 60);
        CSRW(satp, satp);
    } else {
        ERROR("trying to set hs level satp from lower privilege");
    }
}

pte_t vspt_vu[6][PAGE_SIZE/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE)));

void vspt_vu_init(){
    uintptr_t addr;

    addr = 0x00000000;
    for(int i = 0; i < 4; i++){
        vspt_vu[0][i] =
            PTE_V | PTE_U | PTE_AD | PTE_RWX | (addr >> 2);
        addr +=  SUPERPAGE_SIZE(0);
    }

    vspt_vu[0][MEM_BASE/SUPERPAGE_SIZE(0)] =
        PTE_V | (((uintptr_t)&vspt_vu[1][0]) >> 2);

    addr = MEM_BASE;
    for(int i = 0; i < 512; i++) vspt_vu[1][i] = 0;
    for(int i = 0; i <  MEM_SIZE/SUPERPAGE_SIZE(1)/2; i++){
        vspt_vu[1][i] =
           PTE_V | PTE_U | PTE_AD | PTE_RWX | (addr >> 2);
        addr +=  SUPERPAGE_SIZE(1);
    }

    vspt_vu[0][4] =
        PTE_V | (((uintptr_t)&vspt_vu[2][0]) >> 2);

    vspt_vu[2][0] =
        PTE_V  | (((uintptr_t)&vspt_vu[3][0]) >> 2);

    addr = TEST_VPAGE_BASE;
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        vspt_vu[3][i] = (addr >> 2) | PTE_AD |
            test_page_perm_table[i].vs;
        addr +=  PAGE_SIZE;
    }

    vspt_vu[2][1] =
        PTE_V | (((uintptr_t)&vspt_vu[4][0]) >> 2);

    addr = 4 * SUPERPAGE_SIZE(0) + SUPERPAGE_SIZE(1);
    for(int i = 0; i < 512; i++){
        vspt_vu[4][i] = (addr >> 2) |
            PTE_V | PTE_U | PTE_AD | PTE_RWX;
        addr +=  PAGE_SIZE;
    }

    vspt_vu[0][5] =
        PTE_V | (((uintptr_t)&vspt_vu[5][0]) >> 2);

    addr = 5 * SUPERPAGE_SIZE(0);
    for(int i = 0; i < 512; i++){
        vspt_vu[5][i] = (addr >> 2) |
             PTE_V | PTE_U | PTE_AD | PTE_RWX;
        addr +=  SUPERPAGE_SIZE(1);
    }

    uintptr_t satp = (((uintptr_t)vspt_vu) >> 12) | (0x8ULL << 60);
    if(curr_priv == PRIV_VS){
        CSRW(satp, satp);
    } else if(curr_priv == PRIV_HS || curr_priv == PRIV_M){
        CSRW(CSR_VSATP, satp);
    } else {
        ERROR("");
    }
}

// test vu when senvcfg.pmm = 2
bool ssnpm_test_vu_senvcfgpmm2() {

    TEST_START();

    CSRS(senvcfg, MENVCFG_PMM_2);

    uintptr_t addr1 = phys_page_base(VSURWX_GURWX);
    uintptr_t vaddr1 = vs_page_base(VSURWX_GURWX);
    write64(addr1, 0xdeadbeef);

    /**
     * Setup hyp page_tables.
     */
    hspt_hu_init();
    hpt_init();

    /**
     * Setup guest page tables.
     */
    vspt_vu_init();

    goto_priv(PRIV_VU);
    bool check1 = read64(vaddr1) == 0xdeadbeef;
    TEST_ASSERT("vu gets right values", check1);

    TEST_SETUP_EXCEPT();
    uintptr_t vaddr1_1 = vaddr1 | 0x0010000000000000UL;
    read64(vaddr1_1);
    TEST_ASSERT(
        "senvcfg.pmm = 2; load vaddr1_1 should trigger page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_SETUP_EXCEPT();
    write64(vaddr1_1, 0xbeafdead);
    TEST_ASSERT(
        "senvcfg.pmm = 2; store vaddr1_1 should trigger page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    uintptr_t vaddr1_2 = vaddr1 | 0x1000000000000000UL;
    write64(vaddr1_2, 0xbeafdead);
    check1 = read64(vaddr1_2) == 0xbeafdead;
    TEST_ASSERT("senvcfg.pmm = 2; load vaddr1_2 should get right value", check1);
}

// test vu when senvcfg.pmm = 3
bool ssnpm_test_vu_senvcfgpmm3() {

    TEST_START();

    CSRS(senvcfg, MENVCFG_PMM_3);

    uintptr_t addr1 = phys_page_base(VSURWX_GURWX);
    uintptr_t vaddr1 = vs_page_base(VSURWX_GURWX);
    write64(addr1, 0xdeadbeef);

    /**
     * Setup hyp page_tables.
     */
    hspt_hu_init();
    hpt_init();

    /**
     * Setup guest page tables.
     */
    vspt_vu_init();

    goto_priv(PRIV_VU);
    bool check1 = read64(vaddr1) == 0xdeadbeef;
    TEST_ASSERT("vu gets right values", check1);

    TEST_SETUP_EXCEPT();
    uintptr_t vaddr1_1 = vaddr1 | 0x0010000000000000UL;
    write64(vaddr1_1, 0xbeafdead);
    check1 = read64(vaddr1_1) == 0xbeafdead;
    TEST_ASSERT("senvcfg.pmm = 3; load vaddr1_1 should get right value", check1);

    uintptr_t vaddr1_2 = vaddr1 | 0x1000000000000000UL;
    write64(vaddr1_2, 0xdeadbeef);
    check1 = read64(vaddr1_2) == 0xdeadbeef;
    TEST_ASSERT("senvcfg.pmm = 3; load vaddr1_2 should get right value", check1);
}

// test hu when senvcfg.pmm = 2
bool ssnpm_test_hu_senvcfgpmm2() {

    TEST_START();

    CSRS(senvcfg, MENVCFG_PMM_2);

    uintptr_t addr1 = phys_page_base(VSURWX_GURWX);
    uintptr_t vaddr1 = vs_page_base(VSURWX_GURWX);
    write64(addr1, 0xdeadbeef);

    /**
     * Setup hyp page_tables.
     */
    hspt_hu_init();
    hpt_init();

    /**
     * Setup guest page tables.
     */
    vspt_vu_init();

    goto_priv(PRIV_HU);
    bool check1 = read64(vaddr1) == 0xdeadbeef;
    TEST_ASSERT("hu gets right values", check1);

    TEST_SETUP_EXCEPT();
    uintptr_t vaddr1_1 = vaddr1 | 0x0010000000000000UL;
    read64(vaddr1_1);
    TEST_ASSERT(
        "senvcfg.pmm = 2; load vaddr1_1 should trigger page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_SETUP_EXCEPT();
    write64(vaddr1_1, 0xbeafdead);
    TEST_ASSERT(
        "senvcfg.pmm = 2; store vaddr1_1 should trigger page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    uintptr_t vaddr1_2 = vaddr1 | 0x1000000000000000UL;
    write64(vaddr1_2, 0xbeafdead);
    check1 = read64(vaddr1_2) == 0xbeafdead;
    TEST_ASSERT("senvcfg.pmm = 2; load vaddr1_2 should get right value", check1);
}

// test hu when senvcfg.pmm = 3
bool ssnpm_test_hu_senvcfgpmm3() {

    TEST_START();

    CSRS(senvcfg, MENVCFG_PMM_3);

    uintptr_t addr1 = phys_page_base(VSURWX_GURWX);
    uintptr_t vaddr1 = vs_page_base(VSURWX_GURWX);
    write64(addr1, 0xdeadbeef);

    /**
     * Setup hyp page_tables.
     */
    hspt_hu_init();
    hpt_init();

    /**
     * Setup guest page tables.
     */
    vspt_vu_init();

    goto_priv(PRIV_HU);
    bool check1 = read64(vaddr1) == 0xdeadbeef;
    TEST_ASSERT("vu gets right values", check1);

    TEST_SETUP_EXCEPT();
    uintptr_t vaddr1_1 = vaddr1 | 0x0010000000000000UL;
    write64(vaddr1_1, 0xbeafdead);
    check1 = read64(vaddr1_1) == 0xbeafdead;
    TEST_ASSERT("senvcfg.pmm = 3; load vaddr1_1 should get right value", check1);

    uintptr_t vaddr1_2 = vaddr1 | 0x1000000000000000UL;
    write64(vaddr1_2, 0xdeadbeef);
    check1 = read64(vaddr1_2) == 0xdeadbeef;
    TEST_ASSERT("senvcfg.pmm = 3; load vaddr1_2 should get right value", check1);
}

bool ssnpm_test_hint_UMode_hupmm2() {

    TEST_START();

    // hlv & hsv should use hstatus when cpu.iMode = UMode
    CSRS(hstatus, HSTATUS_HUPMM_2);
    // Should not use henvcfg or senvcfg
    CSRS(henvcfg, MENVCFG_PMM_3);
    CSRS(senvcfg, MENVCFG_PMM_3);

    CSRS(hstatus, HSTATUS_HU);

    uintptr_t addr1 = phys_page_base(VSURWX_GURWX);
    uintptr_t vaddr1 = vs_page_base(VSURWX_GURWX);

    /**
     * Setup hyp page_tables.
     */
    hspt_hu_init();
    hpt_init();

    /**
     * Setup guest page tables.
     */
    vspt_vu_init();

    goto_priv(PRIV_HU);

    hsvd(vaddr1, 0xdeadbeef);
    bool check1 = hlvxwu(vaddr1) == 0xdeadbeef;
    TEST_ASSERT("hlvx gets right values", check1);

    TEST_SETUP_EXCEPT();
    uintptr_t vaddr1_1 = vaddr1 | 0x0010000000000000UL;
    hlvd(vaddr1_1);
    TEST_ASSERT(
        "hstatus.hupmm = 2 & cpu.UMode; hlv vaddr1_1 should trigger page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_SETUP_EXCEPT();
    hsvd(vaddr1_1, 0xdeadbeef);
    TEST_ASSERT(
        "hstatus.hupmm = 2 & cpu.UMode; hsv vaddr1_1 should trigger page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    uintptr_t vaddr1_2 = vaddr1 | 0x1000000000000000UL;
    hsvd(vaddr1_2, 0xbeafdead);
    check1 = hlvd(vaddr1_2) == 0xbeafdead;
    TEST_ASSERT("hstatus.hupmm = 2 & cpu.UMode; hlv vaddr1_2 should get right value", check1);
}

bool ssnpm_test_hint_UMode_hupmm3() {

    TEST_START();

    // hlv & hsv should use hstatus when cpu.iMode = UMode
    CSRS(hstatus, HSTATUS_HUPMM_3);

    CSRS(hstatus, HSTATUS_HU);

    uintptr_t addr1 = phys_page_base(VSURWX_GURWX);
    uintptr_t vaddr1 = vs_page_base(VSURWX_GURWX);

    /**
     * Setup hyp page_tables.
     */
    hspt_hu_init();
    hpt_init();

    /**
     * Setup guest page tables.
     */
    vspt_vu_init();

    goto_priv(PRIV_HU);

    hsvd(vaddr1, 0xdeadbeef);
    bool check1 = hlvxwu(vaddr1) == 0xdeadbeef;
    TEST_ASSERT("hlvx gets right values", check1);

    uintptr_t vaddr1_1 = vaddr1 | 0x0010000000000000UL;
    hsvd(vaddr1_1, 0xbeafdead);
    check1 = hlvd(vaddr1_1) == 0xbeafdead;
    TEST_ASSERT("hstatus.hupmm = 3 & cpu.UMode; hlv vaddr1_1 should get right value", check1);

    uintptr_t vaddr1_2 = vaddr1 | 0x1000000000000000UL;
    hsvd(vaddr1_2, 0xdeadbeef);
    check1 = hlvd(vaddr1_2) == 0xdeadbeef;
    TEST_ASSERT("hstatus.hupmm = 3 & cpu.UMode; hlv vaddr1_2 should get right value", check1);
}

// Including:
// 1. HS Mode, hlv & hsv & hlvx insts (Both spvp = 0 or 1)
// 2. VS Mode, normal loads & stores
bool ssnpm_test_SMode() {

    TEST_START();

    // clear these pmm bits of CSRs
    CSRC(mseccfg, MENVCFG_PMM_3);
    CSRC(menvcfg, MENVCFG_PMM_3);
    CSRC(henvcfg, MENVCFG_PMM_3);
    CSRC(hstatus, HSTATUS_HUPMM_3);
    CSRC(senvcfg, MENVCFG_PMM_3);

    // hlv & hsv should use senvcfg when SPVP = 0 & !cpu.UMode
    CSRS(senvcfg, MENVCFG_PMM_2);
    // hlv & hsv should use henvcfg when SPVP = 1
    CSRS(henvcfg, MENVCFG_PMM_3);

    uintptr_t addr1 = phys_page_base(VSURWX_GURWX);
    uintptr_t vaddr1 = vs_page_base(VSURWX_GURWX);
    uintptr_t addr2 = phys_page_base(VSRWX_GURWX);
    uintptr_t vaddr2 = vs_page_base(VSRWX_GURWX);

    /**
     * Setup hyp page_tables.
     */
    hspt_init();
    hpt_init();

    /**
     * Setup guest page tables.
     */
    vspt_init();

    goto_priv(PRIV_HS);
    // Set SPVP = 0
    set_prev_priv(PRIV_VU);

    printf("SET1: HS Stage, senvcfg.pmm = 2, henvcfg.pmm = 3, SPVP = 0\n");

    hsvd(vaddr1, 0xdeadbeef);
    bool check1 = hlvxwu(vaddr1) == 0xdeadbeef;
    TEST_ASSERT("hlvx gets right values", check1);

    TEST_SETUP_EXCEPT();
    uintptr_t vaddr1_1 = vaddr1 | 0x0010000000000000UL;
    hlvd(vaddr1_1);
    TEST_ASSERT(
        "senvcfg.pmm = 2 & spvp = 0 & !cpu.UMode; hlv vaddr1_1 should trigger page fault although henvcfg.pmm = 3",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_SETUP_EXCEPT();
    hsvd(vaddr1_1, 0xdeadbeef);
    TEST_ASSERT(
        "senvcfg.pmm = 2 & spvp = 0 & !cpu.UMode; hsv vaddr1_1 should trigger page fault although henvcfg.pmm = 3",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    uintptr_t vaddr1_2 = vaddr1 | 0x1000000000000000UL;
    hsvd(vaddr1_2, 0xbeafdead);
    check1 = hlvd(vaddr1_2) == 0xbeafdead;
    TEST_ASSERT("senvcfg.pmm = 2 & spvp = 0 & !cpu.UMode; hlv vaddr1_2 should get right value", check1);

    TEST_SETUP_EXCEPT();
    hlvxwu(vaddr1_2);
    TEST_ASSERT(
        "hlvx vaddr1_2 should trigger page fault although senvcfg.pmm = 2 & spvp = 0 & !cpu.UMode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    CSRS(senvcfg, MENVCFG_PMM_3);
    // clear henvcfg.pmm although useless
    CSRC(henvcfg, MENVCFG_PMM_3);

    printf("SET2: HS Stage, senvcfg.pmm = 3, henvcfg.pmm = 0, SPVP = 0\n");

    hsvd(vaddr1, 0xdeadbeef);
    check1 = hlvxwu(vaddr1) == 0xdeadbeef;
    TEST_ASSERT("hlvx gets right values", check1);

    vaddr1_1 = vaddr1 | 0x0010000000000000UL;
    hsvd(vaddr1_1, 0xbeafdead);
    check1 = hlvd(vaddr1_1) == 0xbeafdead;
    TEST_ASSERT("hstatus.hupmm = 3 & spvp = 0 & !cpu.UMode; hlv vaddr1_1 should get right value", check1);

    vaddr1_2 = vaddr1 | 0x1000000000000000UL;
    hsvd(vaddr1_2, 0xdeadbeef);
    check1 = hlvd(vaddr1_2) == 0xdeadbeef;
    TEST_ASSERT("hstatus.hupmm = 3 & spvp = 0 & !cpu.UMode; hlv vaddr1_2 should get right value", check1);

    TEST_SETUP_EXCEPT();
    hlvxwu(vaddr1_2);
    TEST_ASSERT(
        "hlvx vaddr1_2 should trigger page fault although hstatus.hupmm = 3",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    // Set SPVP = 1, Should use henvcfg.pmm
    set_prev_priv(PRIV_VS);
    // clear henvcfg.pmm first
    CSRC(henvcfg, MENVCFG_PMM_3);
    // should not use senvcfg
    CSRS(senvcfg, MENVCFG_PMM_2);

    printf("SET3: HS Stage, senvcfg.pmm = 2, henvcfg.pmm = 0, SPVP = 1\n");

    hsvd(vaddr2, 0xdeadbeef);
    check1 = hlvd(vaddr2) == 0xdeadbeef;
    TEST_ASSERT("vld gets right values", check1);

    TEST_SETUP_EXCEPT();
    uintptr_t vaddr2_1 = vaddr2 | 0x0010000000000000UL;
    hlvd(vaddr2_1);
    TEST_ASSERT(
        "henvcfg.pmm = 0 & spvp = 1; hlv vaddr2_1 should trigger page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_SETUP_EXCEPT();
    hsvd(vaddr2_1, 0xdeadbeef);
    TEST_ASSERT(
        "henvcfg.pmm = 0 & spvp = 1; hsv vaddr2_1 should trigger page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    TEST_SETUP_EXCEPT();
    uintptr_t vaddr2_2 = vaddr2 | 0x1000000000000000UL;
    hsvd(vaddr2_2, 0xbeafdead);
    TEST_ASSERT(
        "henvcfg.pmm = 0 & spvp = 1; hsv vaddr2_2 should trigger page fault although senvcfg.pmm = 2",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    TEST_SETUP_EXCEPT();
    hlvd(vaddr2_2);
    TEST_ASSERT(
        "henvcfg.pmm = 0 & spvp = 1; hlv vaddr2_2 should trigger page fault although senvcfg.pmm = 2",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    // set pmm to 2
    CSRS(henvcfg, MENVCFG_PMM_2);
    // should not use senvcfg
    CSRS(senvcfg, MENVCFG_PMM_3);

    printf("SET4: HS Stage, senvcfg.pmm = 3, henvcfg.pmm = 2, SPVP = 1\n");

    hsvd(vaddr2, 0xdeadbeef);
    check1 = hlvd(vaddr2) == 0xdeadbeef;
    TEST_ASSERT("vld gets right values", check1);

    TEST_SETUP_EXCEPT();
    vaddr2_1 = vaddr2 | 0x0010000000000000UL;
    hlvd(vaddr2_1);
    TEST_ASSERT(
        "henvcfg.pmm = 2 & spvp = 1; hlv vaddr2_1 should trigger page fault although senvcfg.pmm = 3",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_SETUP_EXCEPT();
    hsvd(vaddr2_1, 0xdeadbeef);
    TEST_ASSERT(
        "henvcfg.pmm = 2 & spvp = 1; hlv vaddr2_1 should trigger page fault although senvcfg.pmm = 3",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    TEST_SETUP_EXCEPT();
    vaddr2_2 = vaddr2 | 0x1000000000000000UL;
    hsvd(vaddr2_2, 0xbeafdead);
    check1 = hlvd(vaddr2_2) == 0xbeafdead;
    TEST_ASSERT("henvcfg.pmm = 2 & spvp = 1; hlv vaddr2_2 should get right value", check1);

    // set pmm to 3
    CSRS(henvcfg, MENVCFG_PMM_3);
    // clear senvcfg.pmm although useless
    CSRC(senvcfg, MENVCFG_PMM_3);
    printf("SET5: HS Stage, senvcfg.pmm = 3, henvcfg.pmm = 2, SPVP = 1\n");

    hsvd(vaddr2, 0xdeadbeef);
    check1 = hlvd(vaddr2) == 0xdeadbeef;
    TEST_ASSERT("vld gets right values", check1);

    TEST_SETUP_EXCEPT();
    vaddr2_1 = vaddr2 | 0x0010000000000000UL;
    hsvd(vaddr2_1, 0xdeadbeef);
    check1 = hlvd(vaddr2_1) == 0xdeadbeef;
    TEST_ASSERT("henvcfg.pmm = 3 & spvp = 1; hlv vaddr2_1 should get right value", check1);

    TEST_SETUP_EXCEPT();
    vaddr2_2 = vaddr2 | 0x1000000000000000UL;
    hsvd(vaddr2_2, 0xbeafdead);
    check1 = hlvd(vaddr2_2) == 0xbeafdead;
    TEST_ASSERT("henvcfg.pmm = 3 & spvp = 1; hlv vaddr2_2 should get right value", check1);

    // Test normal load & store in VS Mode

    goto_priv(PRIV_M);

    // clear these pmm bits of CSRs
    CSRC(mseccfg, MENVCFG_PMM_3);
    CSRC(menvcfg, MENVCFG_PMM_3);
    CSRC(henvcfg, MENVCFG_PMM_3);
    CSRC(hstatus, HSTATUS_HUPMM_3);
    CSRC(senvcfg, MENVCFG_PMM_3);

    // should not use csr except for henvcfg
    CSRS(mseccfg, MENVCFG_PMM_3);
    CSRS(menvcfg, MENVCFG_PMM_3);
    CSRS(hstatus, HSTATUS_HUPMM_3);
    CSRS(senvcfg, MENVCFG_PMM_3);

    goto_priv(PRIV_VS);

    printf("SET6: VS Stage, henvcfg.pmm = 0\n");

    TEST_SETUP_EXCEPT();
    read64(vaddr2_1);
    TEST_ASSERT(
        "henvcfg.pmm = 0 in VS Mode; load vaddr2_1 should trigger page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_SETUP_EXCEPT();
    write64(vaddr2_1, 0xdeadbeef);
    TEST_ASSERT(
        "henvcfg.pmm = 0 in VS Mode; store vaddr2_1 should trigger page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

        TEST_SETUP_EXCEPT();
    read64(vaddr2_2);
    TEST_ASSERT(
        "henvcfg.pmm = 0 in VS Mode; load vaddr2_2 should trigger page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_SETUP_EXCEPT();
    write64(vaddr2_2, 0xdeadbeef);
    TEST_ASSERT(
        "henvcfg.pmm = 0 in VS Mode; store vaddr2_2 should trigger page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    goto_priv(PRIV_HS);

    // set pmm to 2
    CSRS(henvcfg, MENVCFG_PMM_2);

    goto_priv(PRIV_VS);

    printf("SET7: VS Stage, henvcfg.pmm = 2\n");

    TEST_SETUP_EXCEPT();
    read64(vaddr2_1);
    TEST_ASSERT(
        "henvcfg.pmm = 2 in VS Mode; load vaddr2_1 should trigger page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_SETUP_EXCEPT();
    write64(vaddr2_1, 0xdeadbeef);
    TEST_ASSERT(
        "henvcfg.pmm = 2 in VS Mode; store vaddr2_1 should trigger page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    TEST_SETUP_EXCEPT();
    write64(vaddr2_2, 0xdeadbeef);
    check1 = read64(vaddr2_2) == 0xdeadbeef;
    TEST_ASSERT("henvcfg.pmm = 2 in VS Mode; load vaddr2_2 should get right value", check1);

    goto_priv(PRIV_HS);

    // set pmm to 3
    CSRS(henvcfg, MENVCFG_PMM_3);

    goto_priv(PRIV_VS);

    printf("SET8: VS Stage, henvcfg.pmm = 3\n");

    TEST_SETUP_EXCEPT();
    write64(vaddr2_1, 0xbeafdead);
    check1 = read64(vaddr2_1) == 0xbeafdead;
    TEST_ASSERT("henvcfg.pmm = 3 in VS Mode; load vaddr2_1 should get right value", check1);

    TEST_SETUP_EXCEPT();
    write64(vaddr2_2, 0xdeadbeef);
    check1 = read64(vaddr2_2) == 0xdeadbeef;
    TEST_ASSERT("henvcfg.pmm = 3 in VS Mode; load vaddr2_2 should get right value", check1);

    TEST_END();
}

// 1. test register write
// 2. whether *tval has been masked
// 3. only stage2 condition
bool ssnpm_test_corner_case() {

    TEST_START();

    goto_priv(PRIV_M);

    // clear these pmm bits of CSRs
    CSRC(mseccfg, MENVCFG_PMM_3);
    CSRC(menvcfg, MENVCFG_PMM_3);
    CSRC(henvcfg, MENVCFG_PMM_3);
    CSRC(hstatus, HSTATUS_HUPMM_3);
    CSRC(senvcfg, MENVCFG_PMM_3);

    printf("SET1: Test register write when reserved\n");

    CSRS(mseccfg, MENVCFG_PMM_RESERVED);
    TEST_ASSERT(
        "mseccfg should keep previous value when write reserved",
        (CSRR(mseccfg) & MENVCFG_PMM_3) == 0
    );

    CSRS(menvcfg, MENVCFG_PMM_RESERVED);
    TEST_ASSERT(
        "menvcfg should keep previous value when write reserved",
        (CSRR(menvcfg) & MENVCFG_PMM_3) == 0
    );

    goto_priv(PRIV_HS);

    CSRS(henvcfg, MENVCFG_PMM_RESERVED);
    TEST_ASSERT(
        "henvcfg should keep previous value when write reserved",
        (CSRR(henvcfg) & MENVCFG_PMM_3) == 0
    );

    CSRS(hstatus, HSTATUS_HUPMM_RESERVED);
    TEST_ASSERT(
        "hstatus should keep previous value when write reserved",
        (CSRR(hstatus) & HSTATUS_HUPMM_RESERVED) == 0
    );

    CSRS(senvcfg, MENVCFG_PMM_RESERVED);
    TEST_ASSERT(
        "senvcfg should keep previous value when write reserved",
        (CSRR(senvcfg) & MENVCFG_PMM_3) == 0
    );

    goto_priv(PRIV_M);

    printf("SET2: Test register CSRS\n");

    CSRS(mseccfg, MENVCFG_PMM_3);
    TEST_ASSERT(
        "mseccfg should write MENVCFG_PMM_3",
        (CSRR(mseccfg) & MENVCFG_PMM_3) == MENVCFG_PMM_3
    );

    CSRS(mseccfg, MENVCFG_PMM_2);
    TEST_ASSERT(
        "mseccfg CSRS MENVCFG_PMM_2 should be MENVCFG_PMM_3",
        (CSRR(mseccfg) & MENVCFG_PMM_3) == MENVCFG_PMM_3
    );

    CSRS(menvcfg, MENVCFG_PMM_3);
    TEST_ASSERT(
        "menvcfg should write MENVCFG_PMM_3",
        (CSRR(menvcfg) & MENVCFG_PMM_3) == MENVCFG_PMM_3
    );

    CSRS(menvcfg, MENVCFG_PMM_2);
    TEST_ASSERT(
        "menvcfg CSRS MENVCFG_PMM_2 should be MENVCFG_PMM_3",
        (CSRR(menvcfg) & MENVCFG_PMM_3) == MENVCFG_PMM_3
    );

    goto_priv(PRIV_HS);

    CSRS(henvcfg, MENVCFG_PMM_3);
    TEST_ASSERT(
        "henvcfg should write MENVCFG_PMM_3",
        (CSRR(henvcfg) & MENVCFG_PMM_3) == MENVCFG_PMM_3
    );

    CSRS(henvcfg, MENVCFG_PMM_2);
    TEST_ASSERT(
        "henvcfg CSRS MENVCFG_PMM_2 should be MENVCFG_PMM_3",
        (CSRR(henvcfg) & MENVCFG_PMM_3) == MENVCFG_PMM_3
    );

    CSRS(hstatus, HSTATUS_HUPMM_3);
    TEST_ASSERT(
        "hstatus should write HSTATUS_HUPMM_3",
        (CSRR(hstatus) & HSTATUS_HUPMM_3) == HSTATUS_HUPMM_3
    );

    CSRS(hstatus, HSTATUS_HUPMM_2);
    TEST_ASSERT(
        "hstatus CSRS HSTATUS_HUPMM_2 should be HSTATUS_HUPMM_3",
        (CSRR(hstatus) & HSTATUS_HUPMM_3) == HSTATUS_HUPMM_3
    );

    CSRS(senvcfg, MENVCFG_PMM_3);
    TEST_ASSERT(
        "senvcfg should write MENVCFG_PMM_3",
        (CSRR(senvcfg) & MENVCFG_PMM_3) == MENVCFG_PMM_3
    );

    CSRS(senvcfg, MENVCFG_PMM_2);
    TEST_ASSERT(
        "senvcfg CSRS MENVCFG_PMM_2 should be MENVCFG_PMM_3",
        (CSRR(senvcfg) & MENVCFG_PMM_3) == MENVCFG_PMM_3
    );

    // load & store of VS Mode should use henvcfg
    // hlv & hsv should also use henvcfg when SPVP = 1
    CSRC(henvcfg, MENVCFG_PMM_3);
    CSRS(henvcfg, MENVCFG_PMM_2);

    uintptr_t addr1 = phys_page_base(VSRWX_GURWX);
    uintptr_t vaddr1 = vs_page_base(VSRWX_GURWX);
    write64(addr1, 0xdeadbeef);

    goto_priv(PRIV_HS);

    /**
     * Setup hyp page_tables.
     */
    hspt_init();
    hpt_init();

    goto_priv(PRIV_VS);

    printf("SET3: Test tval value when only stage2 & pmm = 2\n");

    TEST_SETUP_EXCEPT();
    bool check1 = read64(vaddr1) == 0xdeadbeef;
    TEST_ASSERT("vs gets right values when only stage 2", check1);

    // vaddr1_1[60][56][52] = 1
    uintptr_t vaddr1_1 = vaddr1 | 0x1110000000000000UL;

    TEST_SETUP_EXCEPT();
    read64(vaddr1_1);
    TEST_ASSERT(
        "load when only stage 2; tval should mask high 7 bits to 0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LGPF &&
        excpt.tval == (vaddr1_1 & 0x01ffffffffffffffUL) &&
        excpt.tval2 == ((vaddr1_1 & 0x01ffffffffffffffUL) >> 2)
    );

    // vaddr1_2[56] = 0; [57+] = 1
    uintptr_t vaddr1_2 = vaddr1 | 0xfe10000000000000UL;

    TEST_SETUP_EXCEPT();
    read64(vaddr1_2);
    TEST_ASSERT(
        "load when only stage 2; tval should mask high 7 bits to 0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LGPF &&
        excpt.tval == (vaddr1_2 & 0x01ffffffffffffffUL) &&
        excpt.tval2 == ((vaddr1_2 & 0x01ffffffffffffffUL) >> 2)
    );

    goto_priv(PRIV_HS);
    // Set SPVP = 1, Should use henvcfg.pmm
    set_prev_priv(PRIV_VS);

    TEST_SETUP_EXCEPT();
    hlvd(vaddr1_1);
    TEST_ASSERT(
        "hlv when henvcfg.pmm = 2 in VS Mode; tval should mask high 7 bits to 0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LGPF &&
        excpt.tval == (vaddr1_1 & 0x01ffffffffffffffUL) &&
        excpt.tval2 == ((vaddr1_1 & 0x01ffffffffffffffUL) >> 2)
    );

    TEST_SETUP_EXCEPT();
    hlvd(vaddr1_2);
    TEST_ASSERT(
        "hlv when henvcfg.pmm = 2 in VS Mode; tval should mask high 7 bits to 0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LGPF &
        excpt.tval == (vaddr1_2 & 0x01ffffffffffffffUL) &&
        excpt.tval2 == ((vaddr1_2 & 0x01ffffffffffffffUL) >> 2)
    );

    goto_priv(PRIV_M);

    // set henvcfg.pmm = 3
    CSRS(henvcfg, MENVCFG_PMM_3);

    goto_priv(PRIV_VS);

    printf("SET4: Test tval value when only stage2 & pmm = 3\n");

    TEST_SETUP_EXCEPT();
    check1 = read64(vaddr1) == 0xdeadbeef;
    TEST_ASSERT("vs gets right values when only stage 2", check1);

    // vaddr1_1[60][56][52][48][44] = 1
    vaddr1_1 = vaddr1 | 0x1111100000000000UL;

    TEST_SETUP_EXCEPT();
    read64(vaddr1_1);
    TEST_ASSERT(
        "load when only stage 2; tval should mask high 16 bits to 0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LGPF &&
        excpt.tval == (vaddr1_1 & 0x0000ffffffffffffUL) &&
        excpt.tval2 == ((vaddr1_1 & 0x0000ffffffffffffUL) >> 2)
    );

    // vaddr1_2[47] = 0; [48+] = 1
    vaddr1_2 = vaddr1 | 0xffff100000000000UL;

    TEST_SETUP_EXCEPT();
    read64(vaddr1_2);
    TEST_ASSERT(
        "load when only stage 2; tval should mask high 16 bits to 0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LGPF &&
        excpt.tval == (vaddr1_2 & 0x0000ffffffffffffUL) &&
        excpt.tval2 == ((vaddr1_2 & 0x0000ffffffffffffUL) >> 2)
    );

    goto_priv(PRIV_HS);
    // Set SPVP = 1, Should use henvcfg.pmm
    set_prev_priv(PRIV_VS);

    TEST_SETUP_EXCEPT();
    hlvd(vaddr1_1);
    TEST_ASSERT(
        "hlv when henvcfg.pmm = 3 in VS Mode; tval should mask high 16 bits to 0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LGPF &&
        excpt.tval == (vaddr1_1 & 0x0000ffffffffffffUL) &&
        excpt.tval2 == ((vaddr1_1 & 0x0000ffffffffffffUL) >> 2)
    );

    TEST_SETUP_EXCEPT();
    hlvd(vaddr1_2);
    TEST_ASSERT(
        "hlv when henvcfg.pmm = 3 in VS Mode; tval should mask high 16 bits to 0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LGPF &
        excpt.tval == (vaddr1_2 & 0x0000ffffffffffffUL) &&
        excpt.tval2 == ((vaddr1_2 & 0x0000ffffffffffffUL) >> 2)
    );

    goto_priv(PRIV_HS);

    // set henvcfg.pmm = 2
    CSRC(henvcfg, MENVCFG_PMM_3);
    CSRS(henvcfg, MENVCFG_PMM_2);

    /**
     * Setup guest page tables.
     */
    vspt_init();

    goto_priv(PRIV_VS);

    printf("SET5: Test tval value when both stages & pmm = 2\n");

    TEST_SETUP_EXCEPT();
    check1 = read64(vaddr1) == 0xdeadbeef;
    TEST_ASSERT("vs gets right values when both stages", check1);

    // vaddr1_1[60][56][52] = 1
    vaddr1_1 = vaddr1 | 0x1110000000000000UL;

    TEST_SETUP_EXCEPT();
    read64(vaddr1_1);
    TEST_ASSERT(
        "load when both stages; tval should mask high 7 bits to vaddr1_1[56]",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == (vaddr1_1 | 0xfe00000000000000UL) &&
        excpt.tval2 == 0
    );

    // vaddr1_2[56] = 0; [57+] = 1
    vaddr1_2 = vaddr1 | 0xfe10000000000000UL;

    TEST_SETUP_EXCEPT();
    read64(vaddr1_2);
    TEST_ASSERT(
        "load when both stages; tval should mask high 7 bits to vaddr1_2[56]",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == (vaddr1_2 & 0x01ffffffffffffffUL) &&
        excpt.tval2 == 0
    );

    goto_priv(PRIV_HS);
    // Set SPVP = 1, Should use henvcfg.pmm
    set_prev_priv(PRIV_VS);

    TEST_SETUP_EXCEPT();
    hlvd(vaddr1_1);
    TEST_ASSERT(
        "hlv when henvcfg.pmm = 2 in VS Mode; tval should mask high 7 bits to vaddr1_1[56]",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == (vaddr1_1 | 0xfe00000000000000UL) &&
        excpt.tval2 == 0
    );

    TEST_SETUP_EXCEPT();
    hlvd(vaddr1_2);
    TEST_ASSERT(
        "hlv when henvcfg.pmm = 2 in VS Mode; tval should mask high 7 bits to vaddr1_2[56]",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &
        excpt.tval == (vaddr1_2 & 0x01ffffffffffffffUL) &&
        excpt.tval2 == 0
    );

    TEST_END();
}

bool ssnpm_test_M_HS_Mode() {

    TEST_START();

    uintptr_t addr1 = phys_page_base(VSRWX_GURWX);
    uintptr_t vaddr1 = vs_page_base(VSRWX_GURWX);

    CSRS(mseccfg, MENVCFG_PMM_2);

    printf("SET1: M Mode, mseccfg.pmm = 2\n");

    write64(vaddr1, 0xdeadbeef);
    bool check1 = read64(vaddr1) == 0xdeadbeef;
    TEST_ASSERT("load gets right values", check1);

    TEST_SETUP_EXCEPT();
    uintptr_t vaddr1_1 = vaddr1 | 0x0010000000000000UL;
    read64(vaddr1_1);
    TEST_ASSERT(
        "mseccfg.pmm = 2 & cpu.MMode; load vaddr1_1 should trigger access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    write64(vaddr1_1, 0xdeadbeef);
    TEST_ASSERT(
        "mseccfg.pmm = 2 & cpu.MMode; store vaddr1_1 should trigger access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    uintptr_t vaddr1_2 = vaddr1 | 0x1000000000000000UL;
    write64(vaddr1_2, 0xbeafdead);
    check1 = read64(vaddr1_2) == 0xbeafdead;
    TEST_ASSERT("mseccfg.pmm = 2 & cpu.MMode; load vaddr1_2 should get right value", check1);

    CSRS(mseccfg, MENVCFG_PMM_3);

    printf("SET2: M Mode, mseccfg.pmm = 3\n");

    write64(vaddr1, 0xdeadbeef);
    check1 = read64(vaddr1) == 0xdeadbeef;
    TEST_ASSERT("load gets right values", check1);

    vaddr1_1 = vaddr1 | 0x0010000000000000UL;
    write64(vaddr1_1, 0xbeafdead);
    check1 = read64(vaddr1_1) == 0xbeafdead;
    TEST_ASSERT("mseccfg.pmm = 2 & cpu.MMode; load vaddr1_1 should get right value", check1);

    vaddr1_2 = vaddr1 | 0x1000000000000000UL;
    write64(vaddr1_2, 0xdeadbeef);
    check1 = read64(vaddr1_2) == 0xdeadbeef;
    TEST_ASSERT("mseccfg.pmm = 2 & cpu.MMode; load vaddr1_2 should get right value", check1);

    CSRC(mseccfg, MENVCFG_PMM_3);
    CSRS(menvcfg, MENVCFG_PMM_2);

    /**
     * Setup hyp page_tables.
     */
    hspt_init();

    goto_priv(PRIV_HS);

    printf("SET3: HS Mode, menvcfg.pmm = 2\n");

    write64(vaddr1, 0xdeadbeef);
    check1 = read64(vaddr1) == 0xdeadbeef;
    TEST_ASSERT("load gets right values", check1);

    TEST_SETUP_EXCEPT();
    vaddr1_1 = vaddr1 | 0x0010000000000000UL;
    read64(vaddr1_1);
    TEST_ASSERT(
        "menvcfg.pmm = 2 & cpu.HSMode; load vaddr1_1 should trigger page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_SETUP_EXCEPT();
    write64(vaddr1_1, 0xdeadbeef);
    TEST_ASSERT(
        "menvcfg.pmm = 2 & cpu.HSMode; store vaddr1_1 should trigger page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    vaddr1_2 = vaddr1 | 0x1000000000000000UL;
    write64(vaddr1_2, 0xbeafdead);
    check1 = read64(vaddr1_2) == 0xbeafdead;
    TEST_ASSERT("menvcfg.pmm = 2 & cpu.HSMode; load vaddr1_2 should get right value", check1);

    goto_priv(PRIV_M);

    CSRS(menvcfg, MENVCFG_PMM_3);

    goto_priv(PRIV_HS);

    printf("SET4: HS Mode, menvcfg.pmm = 3\n");

    write64(vaddr1, 0xdeadbeef);
    check1 = read64(vaddr1) == 0xdeadbeef;
    TEST_ASSERT("load gets right values", check1);

    vaddr1_1 = vaddr1 | 0x0010000000000000UL;
    write64(vaddr1_1, 0xbeafdead);
    check1 = read64(vaddr1_1) == 0xbeafdead;
    TEST_ASSERT("menvcfg.pmm = 2 & cpu.HSMode; load vaddr1_1 should get right value", check1);

    vaddr1_2 = vaddr1 | 0x1000000000000000UL;
    write64(vaddr1_2, 0xdeadbeef);
    check1 = read64(vaddr1_2) == 0xdeadbeef;
    TEST_ASSERT("menvcfg.pmm = 2 & cpu.HSMode; load vaddr1_2 should get right value", check1);

    TEST_END();
}
