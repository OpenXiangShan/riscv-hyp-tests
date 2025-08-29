#include <rvh_test.h>
#include <page_tables.h>

TEST_REGISTER_HYP(two_stage_translation);
TEST_REGISTER_HYP(second_stage_only_translation);
TEST_REGISTER_HYP(m_and_hs_using_vs_access);
TEST_REGISTER_HYP(check_xip_regs);
TEST_REGISTER_HYP(check_xie_regs);
TEST_REGISTER_HYP(interrupt_tests);
TEST_REGISTER_HYP(virtual_instruction);
// TEST_REGISTER_HYP(hfence_test); //need that xiangshan run it alone
TEST_REGISTER_HYP(wfi_exception_tests);
TEST_REGISTER_HYP(tinst_tests);
TEST_REGISTER_HYP(apt_tests);
TEST_REGISTER_HYP(tval_inst_tests);
TEST_REGISTER_HYP(cbo_test);
TEST_REGISTER_HYP(test_pbmte);
TEST_REGISTER_HYP(test_pbmt_simple);
TEST_REGISTER_HYP(test_pbmt_forward);
TEST_REGISTER_HYP(test_pbmt_stld_violate);
TEST_REGISTER_HYP(test_pbmt_ldld_violate);
TEST_REGISTER_HYP(test_pbmt_perf);
TEST_REGISTER_HYP(test_pbmt_byte_write_read);
