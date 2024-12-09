#include <page_tables.h>
#include <rvh_test.h>

static inline void touchread(uintptr_t addr){
  asm volatile("" ::: "memory");
  volatile uint64_t x = *(volatile uint64_t *)addr;
}

static inline void touchwrite(uintptr_t addr, uint64_t val){
  asm volatile("" ::: "memory");
  *((volatile uint64_t*) addr) = val;
}

uint64_t __attribute__((section(".nop_test"))) test(void){
  for (int i = 0; i < 1024; i++ ){
    asm volatile ("addi x0, x0, 0");
  }
  return 1;
}

bool test_pbmte() {
  TEST_START();
  INFO("")

  /**
   * Setup hyp page_tables.
   */
  goto_priv(PRIV_HS);
  hspt_init();
  hpt_init();

  /**
   * Setup guest page tables.
   */
  goto_priv(PRIV_VS);
  vspt_init();

  goto_priv(PRIV_M); 
  CSRS(medeleg, (1 << CAUSE_LGPF) | (1 << CAUSE_SGPF));
  CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
  hfence_gvma();
  goto_priv(PRIV_HS);
  CSRS(CSR_HEDELEG, 1 << CAUSE_LPF);
  CSRS(CSR_HENVCFG, HENVCFG_PBMTE);
  hfence_vvma();
  goto_priv(PRIV_VS);

  INFO("test (menvcfg.pbmte, PBMT) = (0, 0)")
  touchread(vs_page_base(VSRWX_GURWX));

  INFO("test (menvcfg.pbmte, PBMT) = (0, 1)");
  TEST_SETUP_EXCEPT();
  touchread(vs_page_base(VSRWX_GURWXP));
  TEST_ASSERT("store page fault triggered", excpt.triggered == true && excpt.cause == CAUSE_LGPF)

  goto_priv(PRIV_M);
  CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
  hfence_gvma();
  goto_priv(PRIV_VS);
  INFO("test (menvcfg.pbmte, PBMT) = (1, 0)");
  touchread(vs_page_base(VSRWX_GURWX));

  INFO("test (menvcfg.pbmte, PBMT) = (1, 1)");
  touchread(vs_page_base(VSRWX_GURWXP));

  TEST_END();
}

bool test_pbmte_withH() {
  TEST_START();
  INFO("\n(menvcfg.PBMTE, henvcfg.PBMTE, PBMT) test cases")
  CSRS(medeleg, 1 << CAUSE_LPF | 1 << CAUSE_LGPF);

  /**
   * Setup hyp page_tables.
   */
  goto_priv(PRIV_HS);
  CSRS(CSR_HEDELEG, 1 << CAUSE_LPF);
  hspt_init();
  hpt_init();

  /**
   * Setup guest page tables.
   */
  goto_priv(PRIV_VS);
  vspt_init();

  INFO("(0, 0, 1)")
  goto_priv(PRIV_M);
  CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
  hfence_gvma();
  goto_priv(PRIV_HS);
  CSRC(CSR_HENVCFG, HENVCFG_PBMTE);
  hfence_vvma();
  goto_priv(PRIV_VS);
  TEST_SETUP_EXCEPT();
  touchread(vs_page_base(VSRWXP_GURWXP));
  TEST_ASSERT("load page fault triggered", excpt.triggered == true && excpt.cause == CAUSE_LPF)

  INFO("(1, 0, 1)")
  goto_priv(PRIV_M);
  CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
  hfence_gvma();
  goto_priv(PRIV_HS);
  CSRC(CSR_HENVCFG, HENVCFG_PBMTE);
  hfence_vvma();
  goto_priv(PRIV_VS);
  TEST_SETUP_EXCEPT();
  touchread(vs_page_base(VSRWXP_GURWXP));
  TEST_ASSERT("load page fault triggered", excpt.triggered == true && excpt.cause == CAUSE_LPF)

  INFO("(0, 1, 1)")
  goto_priv(PRIV_M);
  CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
  hfence_gvma();
  goto_priv(PRIV_HS);
  CSRS(CSR_HENVCFG, HENVCFG_PBMTE);
  hfence_vvma();
  goto_priv(PRIV_VS);
  TEST_SETUP_EXCEPT();
  touchread(vs_page_base(VSRWXP_GURWXP));
  TEST_ASSERT("load page fault triggered", excpt.triggered == true && excpt.cause == CAUSE_LPF)

  INFO("(1, 1, 1)")
  goto_priv(PRIV_M);
  CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
  hfence_gvma();
  goto_priv(PRIV_HS);
  CSRS(CSR_HENVCFG, HENVCFG_PBMTE);
  hfence_vvma();
  goto_priv(PRIV_VS);
  touchread(vs_page_base(VSRWXP_GURWXP));

  TEST_END();
}

bool test_pbmt_PTEs() {
  TEST_START();
  CSRS(medeleg, 1 << CAUSE_LPF | 1 << CAUSE_LGPF | (1 << CAUSE_SGPF));

  /**
   * Setup hyp page_tables.
   */
  goto_priv(PRIV_HS);
  CSRS(CSR_HEDELEG, 1 << CAUSE_LPF);
  hspt_init();
  hpt_init();

  /**
   * Setup guest page tables.
   */
  goto_priv(PRIV_VS);
  vspt_init();

  goto_priv(PRIV_M);
  CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
  hfence_gvma();
  goto_priv(PRIV_HS);
  CSRS(CSR_HENVCFG, HENVCFG_PBMTE);
  hfence_vvma();
  goto_priv(PRIV_VS);

  /**
   * Execute the linked function located in the NC space to test whether it is inferred to execute in the NC space.
   */
  uint64_t (*test_nc)(void) =  (uint64_t (*)(void))vs_page_base(VSRWXP_GURWXP);
  printf("\nNC_vaddr:%lx \t NC_paddr:%lx\n", vs_page_base(VSRWXP_GURWXP), phys_page_base(VSRWXP_GURWXP));
  test_nc();

  TEST_END();
}