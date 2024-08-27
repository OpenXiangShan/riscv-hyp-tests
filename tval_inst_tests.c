#include <rvh_test.h>

bool tval_inst_tests() {

  TEST_START();

  goto_priv(PRIV_M);

  if (false) {
    DETAIL("Test mtval when CAUSE_ILI occurs");

    CSRW(CSR_MEDELEG, 0);

    TEST_SETUP_EXCEPT();
    asm volatile(".4byte 0xFFFFFFFF");
    TEST_ASSERT(
      "check insn 0xFFFFFFFF trap in M mode",
      excpt.triggered == true
    );
    TEST_ASSERT(
      "check insn 0xFFFFFFFF trap in M mode: cause is CAUSE_ILI",
      excpt.cause == CAUSE_ILI
    );
    TEST_ASSERT(
      "check insn 0xFFFFFFFF trap in M mode: mtval is 0xFFFFFFFF",
      excpt.tval == 0xFFFFFFFFUL,
    );
    goto_priv(PRIV_M);
  }

  if (false) {
    DETAIL("Test stval when CAUSE_ILI occurs");

    CSRW(CSR_MEDELEG, 1UL << CAUSE_ILI);
    CSRW(CSR_HEDELEG, 0);
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    asm volatile(".4byte 0xFFFFFFFF");
    TEST_ASSERT(
      "check insn 0xFFFFFFFF trap in HS mode",
      excpt.triggered == true
    );
    TEST_ASSERT(
      "check insn 0xFFFFFFFF trap in HS mode: cause is CAUSE_ILI",
      excpt.cause == CAUSE_ILI
    );
    TEST_ASSERT(
      "check insn 0xFFFFFFFF trap in HS mode: stval is 0xFFFFFFFF",
      excpt.tval == 0xFFFFFFFFUL,
    );
    goto_priv(PRIV_M);
  }

  if (false) {
    DETAIL("Test vstval when CAUSE_ILI occurs");

    CSRW(CSR_MEDELEG, 1UL << CAUSE_ILI);
    CSRW(CSR_HEDELEG, 1UL << CAUSE_ILI);
    goto_priv(PRIV_VS);

    TEST_SETUP_EXCEPT();
    asm volatile(".4byte 0xFFFFFFFF");
    TEST_ASSERT(
      "check insn 0xFFFFFFFF trap in VS mode",
      excpt.triggered == true
    );
    TEST_ASSERT(
      "check insn 0xFFFFFFFF trap in VS mode: cause is CAUSE_ILI",
      excpt.cause == CAUSE_ILI
    );
    TEST_ASSERT(
      "check insn 0xFFFFFFFF trap in VS mode: vstval is 0xFFFFFFFF",
      excpt.tval == 0xFFFFFFFFUL,
    );
    goto_priv(PRIV_M);
  }

  if (false) {
    DETAIL("Test mtval C ext when CAUSE_ILI occurs");

    CSRW(CSR_MEDELEG, 0);
    TEST_SETUP_EXCEPT();

    asm volatile(".2byte 0x8000");
    TEST_ASSERT(
      "check insn 0x8000 trap in M mode",
      excpt.triggered == true
    );
    TEST_ASSERT(
      "check insn 0x8000 trap in M mode: cause is CAUSE_ILI",
      excpt.cause == CAUSE_ILI
    );
    TEST_ASSERT(
      "check insn 0x8000 trap in M mode: mtval is 0x8000",
      excpt.tval == 0x8000UL,
    );
    goto_priv(PRIV_M);
  }

  if (false) {
    DETAIL("Test stval C ext when CAUSE_ILI occurs");

    CSRW(CSR_MEDELEG, 1UL << CAUSE_ILI);
    CSRW(CSR_HEDELEG, 0);
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    asm volatile(".2byte 0x8000");
    TEST_ASSERT(
      "check insn 0x8000 trap in HS mode",
      excpt.triggered == true
    );
    TEST_ASSERT(
      "check insn 0x8000 trap in HS mode: cause is CAUSE_ILI",
      excpt.cause == CAUSE_ILI
    );
    TEST_ASSERT(
      "check insn 0x8000 trap in HS mode: stval is 0x8000",
      excpt.tval == 0x8000UL,
    );
    goto_priv(PRIV_M);
  }

  if (false) {
    DETAIL("Test vstval C ext when CAUSE_ILI occurs");

    CSRW(CSR_MEDELEG, 1UL << CAUSE_ILI);
    CSRW(CSR_HEDELEG, 1UL << CAUSE_ILI);
    goto_priv(PRIV_VS);

    TEST_SETUP_EXCEPT();
    asm volatile(".2byte 0x8000");
    TEST_ASSERT(
      "check insn 0x8000 trap in VS mode",
      excpt.triggered == true
    );
    TEST_ASSERT(
      "check insn 0x8000 trap in VS mode: cause is CAUSE_ILI",
      excpt.cause == CAUSE_ILI
    );
    TEST_ASSERT(
      "check insn 0x8000 trap in VS mode: vstval is 0x8000",
      excpt.tval == 0x8000UL,
    );
    goto_priv(PRIV_M);
  }

  if (true) {
    DETAIL("Test mtval when CAUSE_VRTI occurs");

    CSRW(CSR_MEDELEG, 0);
    goto_priv(PRIV_VS);

    TEST_SETUP_EXCEPT();
    hfence_vvma();

    TEST_ASSERT(
      "check insn hfence.vvma executed in VS mode and traping in M mode",
      excpt.triggered == true
    );
    TEST_ASSERT(
      "check insn hfence.vvma executed in VS mode and traping in M mode: cause is CAUSE_VRTI",
      excpt.cause == CAUSE_VRTI
    );
    TEST_ASSERT(
      "check insn hfence.vvma executed in VS mode and traping in M mode: mtval is 0x22000073",
      excpt.tval == 0x22000073UL,
    );
    goto_priv(PRIV_M);
  }

  if (true) {
    DETAIL("Test stval when CAUSE_VRTI occurs");

    CSRW(CSR_MEDELEG, (1UL << CAUSE_VRTI));
    CSRW(CSR_HEDELEG, (1UL << CAUSE_VRTI));
    // CAUSE_VRTI cannot be delegated to VS mode
    TEST_ASSERT(
      "hedeleg.EX_VI should be false",
      CSRR(CSR_HEDELEG) == 0
    );
    goto_priv(PRIV_VS);

    TEST_SETUP_EXCEPT();
    hfence_vvma();

    TEST_ASSERT(
      "check insn hfence.vvma executed in VS mode and traping in HS mode",
      excpt.triggered == true
    );
    TEST_ASSERT(
      "check insn hfence.vvma executed in VS mode and traping in HS mode: cause is CAUSE_VRTI",
      excpt.cause == CAUSE_VRTI
    );
    TEST_ASSERT(
      "check insn hfence.vvma executed in VS mode and traping in HS mode: stval is 0x22000073",
      excpt.tval == 0x22000073UL,
    );
    goto_priv(PRIV_M);
  }

  TEST_END();
}
