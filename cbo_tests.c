#include <page_tables.h>
#include <rvh_test.h>

#define ADDR_ZERO 0

bool cbo_test() {

  TEST_START();

  // close all delegations of exceptions
  CSRW(CSR_MEDELEG, 0);
  CSRW(CSR_HEDELEG, 0);

  uintptr_t vaddr_f = vs_page_base(SWITCH1);
  uint64_t value = 0xdeadbeef;

  //////////////////////////////////////////////////////////////////////

  if (true) {
    INFO("test menvcfg.CBCFE off and execute cbo in MODE_U")
    // open all enable bits
    CSRW(CSR_MENVCFG, (uint64_t)-1);
    CSRW(CSR_HENVCFG, (uint64_t)-1);
    CSRW(CSR_SENVCFG, (uint64_t)-1);
    // only disable menvcfg.CBCFE
    CSRC(CSR_MENVCFG, MENVCFG_CBCFE);
    // goto tested MODE
    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    cbo_clean(ADDR_ZERO);
    TEST_ASSERT("U-mode cbo.clean causes EX_II when menvcfg.CBCFE=0",
                excpt.triggered == true && excpt.cause == CAUSE_ILI);

    TEST_SETUP_EXCEPT();
    cbo_flush(ADDR_ZERO);
    TEST_ASSERT("U-mode cbo.flush causes EX_II when menvcfg.CBCFE=0",
                excpt.triggered == true && excpt.cause == CAUSE_ILI);

    // TODO: cbo.inval and cbo.zero no exception checks
    goto_priv(PRIV_M);
  }

  if (true) {
    INFO("test menvcfg.CBZE off and execute cbo in MODE_U")
    // open all enable bits
    CSRW(CSR_MENVCFG, (uint64_t)-1);
    CSRW(CSR_HENVCFG, (uint64_t)-1);
    CSRW(CSR_SENVCFG, (uint64_t)-1);
    // only disable menvcfg.CBZE
    CSRC(CSR_MENVCFG, MENVCFG_CBZE);
    // goto tested MODE
    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    cbo_zero(ADDR_ZERO);
    TEST_ASSERT("U-mode cbo.zero causes EX_II when menvcfg.CBZE=0",
                excpt.triggered == true && excpt.cause == CAUSE_ILI);
    // TODO: cbo.clean, cbo.flush, cbo.inval no exception checks

    goto_priv(PRIV_M);
  }

  if (true) {
    INFO("test menvcfg.CBIE off and execute cbo in MODE_U")
    // open all enable bits
    CSRW(CSR_MENVCFG, (uint64_t)-1);
    CSRW(CSR_HENVCFG, (uint64_t)-1);
    CSRW(CSR_SENVCFG, (uint64_t)-1);
    // only disable menvcfg.CBIE
    CSRC(CSR_MENVCFG, MENVCFG_CBIE);
    // goto tested MODE
    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    cbo_inval(ADDR_ZERO);
    TEST_ASSERT("U-mode cbo.inval causes EX_II when menvcfg.CBIE=0",
                excpt.triggered == true && excpt.cause == CAUSE_ILI);
    // TODO: cbo.clean, cbo.flush, cbo.zero no exception checks

    goto_priv(PRIV_M);
  }


  //////////////////////////////////////////////////////////////////////

  TEST_END();
}