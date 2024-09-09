#include <rvh_test.h>

bool check_xie_read(int MIDELEG, int MVIEN, int HIDELEG, int HVIEN);
bool check_xip_read(int STCE, int MIDELEG, int MVIEN, int HIDELEG, int HVIEN);

bool check_xie_regs() {
    TEST_START();

    CSRW(CSR_MIDELEG, 0);

    int64_t mtime_mask = ~((int64_t)0x80);

    for (int iMIDELEG = 0; iMIDELEG < 2; iMIDELEG ++) {
        for (int iMVIEN = 0; iMVIEN < 2; iMVIEN ++) {
            for (int iHIDELEG = 0; iHIDELEG < 2; iHIDELEG ++) {
                for (int iHVIEN = 0; iHVIEN < 2; iHVIEN ++) {
                    check_xie_read(iMIDELEG, iMVIEN, iHIDELEG, iHVIEN);
                }
            }
        }
    }

    VERBOSE("setting mideleg and hideleg\n");
    CSRW(CSR_MIDELEG, (uint64_t)-1);
    CSRW(CSR_HIDELEG, (uint64_t)-1);
    check_csr_wrrd("vsie", CSR_VSIE, (uint64_t) -1, 0x2222);
    
    VERBOSE("setting all in mie\n");
    CSRW(mie, (uint64_t)-1);
    check_csr_rd("hie", CSR_HIE, 0x1444);
    check_csr_rd("sie", CSR_SIE, 0x2222);
    check_csr_rd("vsie", CSR_VSIE, 0x2222);
    goto_priv(PRIV_VS);
    check_csr_rd("sie (vs perspective)", CSR_SIE, 0x2222);
    goto_priv(PRIV_M);

    VERBOSE("clearing all in mie\n");
    CSRW(CSR_MIE, (uint64_t)0);
    check_csr_rd("hie", CSR_HIE, 0x0);
    check_csr_rd("sie", CSR_SIE, 0x0);
    check_csr_rd("vsie", CSR_VSIE, 0x0);
    goto_priv(PRIV_VS);
    check_csr_rd("sie (vs perspective)", CSR_SIE, 0x0);
    goto_priv(PRIV_M);

    VERBOSE("clearing all in mideleg\n");
    CSRW(CSR_MIDELEG, (uint64_t)0);
    VERBOSE("setting all in mvien\n");
    CSRW(CSR_MVIEN, (uint64_t)-1);
    check_csr_rd("mvien", CSR_MVIEN, 0xffffffffffffe202);
    VERBOSE("setting all in mie\n");
    CSRW(CSR_MIE, (uint64_t)-1);
    check_csr_rd("hie", CSR_HIE, 0x1444);
    check_csr_rd("sie", CSR_SIE, 0x0);
    check_csr_rd("vsie", CSR_VSIE, 0x222);

    VERBOSE("setting all in sie\n");
    CSRW(CSR_SIE, (uint64_t)-1);
    check_csr_rd("sie", CSR_SIE, 0xffffffffffffe202);
    check_csr_rd("vsie", CSR_VSIE, 0x2222);
    goto_priv(PRIV_VS);
    check_csr_rd("sie (vs perspective)", CSR_SIE, 0x2222);
    goto_priv(PRIV_M);

    VERBOSE("clearing all in mie\n");
    CSRW(CSR_MIE, (uint64_t)0);
    check_csr_rd("hie", CSR_HIE, 0x0);
    check_csr_rd("sie", CSR_SIE, 0xffffffffffffe202);
    check_csr_rd("vsie", CSR_VSIE, 0x2000);
    goto_priv(PRIV_VS);
    check_csr_rd("sie (vs perspective)", CSR_SIE, 0x2000);
    goto_priv(PRIV_M);

    VERBOSE("clearing all in sie\n");
    CSRW(CSR_SIE, (uint64_t)0);
    check_csr_rd("sie", CSR_SIE, 0x0);
    check_csr_rd("vsie", CSR_VSIE, 0x0);
    goto_priv(PRIV_VS);
    check_csr_rd("sie (vs perspective)", CSR_SIE, 0x0);
    goto_priv(PRIV_M);

    VERBOSE("clearing all in hideleg");
    CSRW(CSR_HIDELEG, (uint64_t)0);
    VERBOSE("setting all in hvien");
    CSRW(CSR_HVIEN, (uint64_t)-1);
    check_csr_wrrd("vsie", CSR_VSIE, (uint64_t)-1, 0xffffffffffffe000);

    TEST_END();
}

bool check_xip_regs(){

    TEST_START();

    for (int iSTCE = 0; iSTCE < 2; iSTCE ++) {
        for (int iMIDELEG = 0; iMIDELEG < 2; iMIDELEG ++) {
            for (int iMVIEN = 0; iMVIEN < 2; iMVIEN ++) {
                for (int iHIDELEG = 0; iHIDELEG < 2; iHIDELEG ++) {
                    for (int iHVIEN = 0; iHVIEN < 2; iHVIEN ++) {
                        check_xip_read(iSTCE, iMIDELEG, iMVIEN, iHIDELEG, iHVIEN);
                    }
                }
            }
        }
    }

    int64_t mtime_mask = ~((int64_t)0x80);

    CSRW(mideleg, (uint64_t)-1);
    VERBOSE("setting mideleg and hideleg\n");
    CSRW(CSR_HIDELEG, (uint64_t)-1);
    check_csr_wrrd("vsip", CSR_VSIP, (uint64_t) -1, 0x2);
    check_csr_wrrd("vsie", CSR_VSIE, (uint64_t) -1, 0x222);

    VERBOSE("setting all in mip\n");
    CSRW(mip, (uint64_t)-1);
    check_csr_rd("hip", CSR_HIP, 0x4);
    check_csr_rd("sip", sip, 0x222);
    // check_csr_rd_mask("mip", mip, 0x226, mtime_mask); // only test when nemu don't use difftest because spike, as ref, shut up time interrupt
    check_csr_rd("vsip", CSR_VSIP, 0x2);
    goto_priv(PRIV_VS);
    check_csr_rd("sip (vs perspective)", sip, 0x2);
    goto_priv(PRIV_M);

    VERBOSE("clearing all in mip\n");
    CSRW(mip, (uint64_t)0);
    check_csr_rd("hip", CSR_HIP, 0x0);
    check_csr_rd("sip", sip, 0x0);
    // check_csr_rd_mask("mip", mip, 0x000, mtime_mask);
    check_csr_rd("vsip", CSR_VSIP, 0x0);
    goto_priv(PRIV_VS);
    check_csr_rd("sip (vs perspective)", sip, 0x0);
    goto_priv(PRIV_M);   

    VERBOSE("setting all in hvip\n");
    CSRW(CSR_HVIP, (uint64_t)-1);
    check_csr_rd("hvip", CSR_HVIP, 0x444);
    check_csr_rd("hip", CSR_HIP, 0x444);
    check_csr_rd("sip", sip, 0x0);
    // check_csr_rd_mask("mip", mip, 0x444, mtime_mask);
    check_csr_rd("vsip", CSR_VSIP, 0x222);
    goto_priv(PRIV_VS);
    check_csr_rd("sip (vs perspective)", sip, 0x222);
    goto_priv(PRIV_M);

    VERBOSE("clearing all in hvip\n");
    CSRW(CSR_HVIP, (uint64_t)0);
    check_csr_rd("hip", CSR_HIP, 0x0);
    check_csr_rd("sip", sip, 0x0);
    // check_csr_rd_mask("mip", mip, 0x000, mtime_mask);
    check_csr_rd("vsip", CSR_VSIP, 0x0);
    goto_priv(PRIV_VS);
    check_csr_rd("sip (vs perspective)", sip, 0x0);
    goto_priv(PRIV_M);

    TEST_END();
}

bool interrupt_tests(){

    TEST_START();

    /**
     * Test trigerring VSSI without delegating it. 
     * It assumes it is already delegated in miedeleg (it should be hardwired)
     */

    goto_priv(PRIV_HS);
    CSRC(sstatus, SSTATUS_SPIE_BIT | SSTATUS_SIE_BIT);
    CSRS(CSR_HIE, 0x4);
    CSRS(CSR_HIP, 0x4);
    TEST_SETUP_EXCEPT();
    goto_priv(PRIV_VS);
    //CSRS(sstatus, SSTATUS_SIE_BIT);
    TEST_ASSERT("vs sw irq with no delegation", 
        excpt.triggered && excpt.cause == CAUSE_VSSI && excpt.priv == PRIV_HS);
   
    /**
     * Test trigerring VSSI and delegating it. Should trap to VS with cause SSI.
     * It assumes it is already delegated in miedeleg (it should be hardwired)
     */
    goto_priv(PRIV_HS);
    CSRS(CSR_HIDELEG, 0x4);
    CSRS(CSR_HIP, 0x4);
    TEST_SETUP_EXCEPT();
    goto_priv(PRIV_VS);
    CSRS(sie, 0x2);
    CSRS(sstatus, 0x2);
    TEST_ASSERT("vs sw irq with delegation", 
        excpt.triggered && excpt.cause == CAUSE_SSI && excpt.priv == PRIV_VS);

    TEST_END();
}

bool check_xip_read(
    int STCE, 
    int MIDELEG,
    int MVIEN,
    int HIDELEG,
    int HVIEN
) {
    CSRW(CSR_MENVCFG, !STCE    ? 0 : (1UL << 63));
    CSRW(CSR_MIDELEG, !MIDELEG ? 0 : -1);
    CSRW(CSR_MVIEN,   !MVIEN   ? 0 : -1);
    CSRW(CSR_HIDELEG, !HIDELEG ? 0 : -1);
    CSRW(CSR_HVIEN,   !HVIEN   ? 0 : -1);

    INFO("Check all xip, when")
    INFO("  menvcfg=%#lx", CSRR(CSR_MENVCFG));
    INFO("  mideleg=%#lx", CSRR(CSR_MIDELEG));
    INFO("  mvien  =%#lx", CSRR(CSR_MVIEN));
    INFO("  hideleg=%#lx", CSRR(CSR_HIDELEG));
    INFO("  hvien  =%#lx", CSRR(CSR_HVIEN));
    INFO("");

    CSRW(mip, (uint64_t)-1);
    INFO("  set mip all 1s, get %#lx", CSRR(mip));
    CSRW(mip, 0);

    CSRW(CSR_MVIP, (uint64_t)-1);
    INFO("  set mvip all 1s, get %#lx", CSRR(CSR_MVIP));
    CSRW(CSR_MVIP, 0);

    CSRW(CSR_HIP, (uint64_t)-1);
    INFO("  set hip all 1s, get %#lx", CSRR(CSR_HIP));
    CSRW(CSR_HIP, 0);

    CSRW(CSR_HVIP, (uint64_t)-1);
    INFO("  set hvip all 1s, get %#lx", CSRR(CSR_HVIP));
    CSRW(CSR_HVIP, 0);

    CSRW(CSR_SIP, (uint64_t)-1);
    INFO("  set sip all 1s, get %#lx", CSRR(CSR_SIP));
    CSRW(CSR_SIP, 0);

    CSRW(CSR_VSIP, (uint64_t)-1);
    INFO("  set vsip all 1s, get %#lx", CSRR(CSR_VSIP));
    CSRW(CSR_VSIP, 0);

    return true;
}

bool check_xie_read(
    int MIDELEG,
    int MVIEN,
    int HIDELEG,
    int HVIEN
) {
    CSRW(CSR_MIDELEG, !MIDELEG ? 0 : -1);
    CSRW(CSR_MVIEN,   !MVIEN   ? 0 : -1);
    CSRW(CSR_HIDELEG, !HIDELEG ? 0 : -1);
    CSRW(CSR_HVIEN,   !HVIEN   ? 0 : -1);

    INFO("Check all xie, when")
    INFO("  mideleg=%#lx", CSRR(CSR_MIDELEG));
    INFO("  mvien  =%#lx", CSRR(CSR_MVIEN));
    INFO("  hideleg=%#lx", CSRR(CSR_HIDELEG));
    INFO("  hvien  =%#lx", CSRR(CSR_HVIEN));
    INFO("");

    CSRW(CSR_MIE, (uint64_t)-1);
    INFO("  set mie all 1s, get %#lx", CSRR(CSR_MIE));
    CSRW(CSR_MIE, 0);

    CSRW(CSR_MVIEN, (uint64_t)-1);
    INFO("  set mvien all 1s, get %#lx", CSRR(CSR_MVIEN));
    CSRW(CSR_MVIEN, 0);

    CSRW(CSR_HVIEN, (uint64_t)-1);
    INFO("  set hvien all 1s, get %#lx", CSRR(CSR_HVIEN));
    CSRW(CSR_HVIEN, 0);

    CSRW(CSR_HIE, (uint64_t)-1);
    INFO("  set hie all 1s, get %#lx", CSRR(CSR_HIE));
    CSRW(CSR_HIE, 0);

    CSRW(CSR_SIE, (uint64_t)-1);
    INFO("  set sie all 1s, get %#lx", CSRR(CSR_SIE));
    CSRW(CSR_SIE, 0);

    CSRW(CSR_VSIE, (uint64_t)-1);
    INFO("  set vsie all 1s, get %#lx", CSRR(CSR_VSIE));
    CSRW(CSR_VSIE, 0);

    return true;
}
