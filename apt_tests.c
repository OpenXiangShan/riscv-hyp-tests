#include <rvh_test.h>
#include <page_tables.h>

#define PaddrWidth  (48)
#define InPageWidth (12)
#define VmidWidth   (14)
#define AsidWidth   (16)
#define Sv39x4Width (39 + 2)
#define Sv48x4Width (48 + 2)

const uint64_t origin_ppn = 0xfffffffffff;
const uint64_t BARE_MASK = BIT_MASK(0, PaddrWidth - InPageWidth);
const uint64_t SV39X4_MASK = BIT_MASK(0, Sv39x4Width - InPageWidth);
const uint64_t SV48X4_MASK = BIT_MASK(0, Sv48x4Width - InPageWidth);
const uint64_t paddr_limited_right_satp_ppn   = origin_ppn & BARE_MASK;
const uint64_t paddr_limited_right_hgatp_ppn  = origin_ppn & BARE_MASK & ~(uint64_t)(0x3);
const uint64_t bare_limited_right_vsatp_ppn   = origin_ppn & BARE_MASK;
const uint64_t sv39x4_limited_right_vsatp_ppn = origin_ppn & SV39X4_MASK;
const uint64_t sv48x4_limited_right_vsatp_ppn = origin_ppn & SV48X4_MASK;
const uint64_t ppn_dead_beef = 0xdeadbeef;
const uint64_t bare_limited_vsatp_ppn_dead_beef = ppn_dead_beef & BARE_MASK;
const uint64_t sv39x4_limited_vsatp_ppn_dead_beef = ppn_dead_beef & SV39X4_MASK;
const uint64_t sv48x4_limited_vsatp_ppn_dead_beef = ppn_dead_beef & SV48X4_MASK;

typedef enum APT_MODE {
  MODE_BARE   = 0x0,
  MODE_ILL    = 0x1,
  MODE_SV39   = 0x8,
  MODE_SV48   = 0x9,
  MODE_SV39x4 = 0x8,
  MODE_SV48x4 = 0x9,
} APT_MODE;

uint64_t make_apt(APT_MODE mode, unsigned id, uint64_t ppn) {
  return ((uint64_t)mode << 60) | ((uint64_t)id << 44) << ppn;
}

bool apt_tests() {

  TEST_START();

  goto_priv(PRIV_M);

  CSRW(CSR_MEDELEG, 0);

  DETAIL("Test hgatp");
  if (true) {
    {
      CSRW(CSR_SATP,  make_apt(MODE_SV39, 0, 0));
      CSRW(CSR_VSATP, make_apt(MODE_SV39, 0, 0));
      check_csr_wrrd("hgatp Bare",                           CSR_HGATP, make_apt(MODE_BARE,      0,          0), make_apt(MODE_BARE,      0,                             0));
      check_csr_wrrd("hgatp Bare VMID",                      CSR_HGATP, make_apt(MODE_BARE, 0x3FFF,          0), make_apt(MODE_BARE, 0x3FFF,                             0));
      check_csr_wrrd("hgatp Bare PPN",                       CSR_HGATP, make_apt(MODE_BARE,      0, origin_ppn), make_apt(MODE_BARE,      0, paddr_limited_right_hgatp_ppn));
    }

    {
      CSRW(CSR_SATP,  make_apt(MODE_BARE, 0, 0));
      CSRW(CSR_VSATP, make_apt(MODE_BARE, 0, 0));
      check_csr_wrrd("hgatp Sv39x4",                         CSR_HGATP, make_apt(MODE_SV39x4,      0,          0), make_apt(MODE_SV39x4,      0,                             0));
      check_csr_wrrd("hgatp Sv39x4 VMID",                    CSR_HGATP, make_apt(MODE_SV39x4, 0x3FFF,          0), make_apt(MODE_SV39x4, 0x3FFF,                             0));
      check_csr_wrrd("hgatp Sv39x4 PPN width limits",        CSR_HGATP, make_apt(MODE_SV39x4,      0, origin_ppn), make_apt(MODE_SV39x4,      0, paddr_limited_right_hgatp_ppn));

      check_csr_wrrd("hgatp Sv48x4",                         CSR_HGATP, make_apt(MODE_SV48x4,      0,          0), make_apt(MODE_SV48x4,      0,                             0));
      check_csr_wrrd("hgatp Sv48x4 VMID",                    CSR_HGATP, make_apt(MODE_SV48x4, 0x3FFF,          0), make_apt(MODE_SV48x4, 0x3FFF,                             0));
      check_csr_wrrd("hgatp Sv48x4 PPN width limits",        CSR_HGATP, make_apt(MODE_SV48x4,      0, origin_ppn), make_apt(MODE_SV48x4,      0, paddr_limited_right_hgatp_ppn));

      CSRW(CSR_HGATP, make_apt(MODE_SV39x4, 0, 0));
      check_csr_wrrd("hgatp mode(ill, keep)",                CSR_HGATP, make_apt(MODE_ILL   ,      0,          0), make_apt(MODE_SV39x4,      0,                             0));
      check_csr_wrrd("hgatp mode(ill, keep) VMID(change)",   CSR_HGATP, make_apt(MODE_ILL   , 0x3FFF,          0), make_apt(MODE_SV39x4, 0x3FFF,                             0));
      check_csr_wrrd("hgatp mode(ill) PPN width limits",     CSR_HGATP, make_apt(MODE_ILL   ,      0, origin_ppn), make_apt(MODE_SV39x4,      0, paddr_limited_right_hgatp_ppn));
    }
  }

  DETAIL("Test satp");
  if (true) {
    {
      CSRW(CSR_HGATP, make_apt(MODE_SV39x4, 0, 0));
      CSRW(CSR_VSATP, make_apt(MODE_SV39  , 0, 0));
      check_csr_wrrd("satp Bare",                        CSR_SATP, make_apt(MODE_BARE,      0,          0), make_apt(MODE_BARE,      0,                             0));
    }
    {
      CSRW(CSR_HGATP, make_apt(MODE_BARE, 0, 0));
      CSRW(CSR_VSATP, make_apt(MODE_BARE, 0, 0));
      check_csr_wrrd("satp Sv39",                        CSR_SATP, make_apt(MODE_SV39,      0,          0), make_apt(MODE_SV39,      0,                             0));
      check_csr_wrrd("satp Sv39 ASID",                   CSR_SATP, make_apt(MODE_SV39, 0xFFFF,          0), make_apt(MODE_SV39, 0xFFFF,                             0));
      check_csr_wrrd("satp Sv39 PPN width limits",       CSR_SATP, make_apt(MODE_SV39,      0, origin_ppn), make_apt(MODE_SV39,      0,  paddr_limited_right_satp_ppn));

      check_csr_wrrd("satp Sv48",                        CSR_SATP, make_apt(MODE_SV48,      0,          0), make_apt(MODE_SV48,      0,                             0));
      check_csr_wrrd("satp Sv48 ASID",                   CSR_SATP, make_apt(MODE_SV48, 0xFFFF,          0), make_apt(MODE_SV48, 0xFFFF,                             0));
      check_csr_wrrd("satp Sv48 PPN width limits",       CSR_SATP, make_apt(MODE_SV48,      0, origin_ppn), make_apt(MODE_SV48,      0,  paddr_limited_right_satp_ppn));

      CSRW(CSR_SATP, make_apt(MODE_SV39, 0, origin_ppn));
      check_csr_wrrd("satp mode(illegal) others(keep)",  CSR_SATP, make_apt(MODE_ILL ,      0,          0), make_apt(MODE_SV39,      0,  paddr_limited_right_satp_ppn));
    }
  }

  DETAIL("Test vsatp when hgatp.MODE=Bare");
  if (true) {
    // set hgatp.mode = Bare
    CSRW(CSR_HGATP, make_apt(MODE_BARE, 0, 0));
    {
      CSRW(CSR_SATP, make_apt(MODE_SV39, 0, 0));
      check_csr_wrrd("vsatp Bare",                        CSR_VSATP, make_apt(MODE_BARE,      0,          0), make_apt(MODE_BARE,      0,                             0));
    }
    {
      CSRW(CSR_SATP, make_apt(MODE_BARE, 0, 0));
      check_csr_wrrd("vsatp Sv39",                        CSR_VSATP, make_apt(MODE_SV39,      0,          0), make_apt(MODE_SV39,      0,                            0));
      check_csr_wrrd("vsatp Sv39 ASID",                   CSR_VSATP, make_apt(MODE_SV39, 0xFFFF,          0), make_apt(MODE_SV39, 0xFFFF,                            0));
      check_csr_wrrd("vsatp Sv39 PPN width limits",       CSR_VSATP, make_apt(MODE_SV39,      0, origin_ppn), make_apt(MODE_SV39,      0, bare_limited_right_vsatp_ppn));

      check_csr_wrrd("vsatp Sv48",                        CSR_VSATP, make_apt(MODE_SV48,      0,          0), make_apt(MODE_SV48,      0,                            0));
      check_csr_wrrd("vsatp Sv48 ASID",                   CSR_VSATP, make_apt(MODE_SV48, 0xFFFF,          0), make_apt(MODE_SV48, 0xFFFF,                            0));
      check_csr_wrrd("vsatp Sv48 PPN width limits",       CSR_VSATP, make_apt(MODE_SV48,      0, origin_ppn), make_apt(MODE_SV48,      0, bare_limited_right_vsatp_ppn));

      CSRW(CSR_VSATP, make_apt(MODE_SV39, 0xFFFF, origin_ppn));
      // check vsatp writed with MODE_ILL but other fields writed as normal WARL in Mode M
      check_csr_wrrd("vsatp mode(ill) others(WARL) in MODE_M",  CSR_VSATP, make_apt(MODE_ILL, 0, ppn_dead_beef), make_apt(MODE_SV39, 0, bare_limited_vsatp_ppn_dead_beef))

      CSRW(CSR_VSATP, make_apt(MODE_BARE, 0, 0));

      DEBUG("hgatp: %lx", CSRR(CSR_HGATP));
      DEBUG("vsatp: %lx", CSRR(CSR_VSATP));
      DEBUG(" satp: %lx", CSRR(CSR_SATP));
      DEBUG("medeleg: %lx", CSRR(CSR_MEDELEG));

      goto_priv(PRIV_VS);

      CSRW(CSR_SATP, make_apt(MODE_BARE, 0, 0));
      DEBUG("vsatp: %lx", CSRR(CSR_SATP));
      // check vsatp writed with MODE_ILL but other fields keep old values in Mode VS
      check_csr_wrrd("vsatp mode(ill) others(keep) in MODE_VS",  CSR_SATP, make_apt(MODE_ILL, 0xFFFF, origin_ppn), make_apt(MODE_BARE, 0, 0));
    }
  }

  DETAIL("Test vsatp when hgatp.MODE=Sv39x4")
  if (true) {
    goto_priv(PRIV_M);
    // set hgatp.mode = Sv39
    CSRW(CSR_HGATP, make_apt(MODE_SV39x4, 0, 0));

    {
      CSRW(CSR_SATP, make_apt(MODE_SV39, 0, 0));
      DEBUG("hgatp=%lx", CSRR(CSR_HGATP));
      check_csr_wrrd("vsatp Bare",                        CSR_VSATP, make_apt(MODE_BARE, 0, 0), make_apt(MODE_BARE, 0, 0));
    }

    {
      CSRW(CSR_SATP, make_apt(MODE_BARE, 0, 0));
      check_csr_wrrd("vsatp Sv39",                        CSR_VSATP, make_apt(MODE_SV39,      0,          0), make_apt(MODE_SV39,      0,                              0));
      check_csr_wrrd("vsatp Sv39 ASID",                   CSR_VSATP, make_apt(MODE_SV39, 0xFFFF,          0), make_apt(MODE_SV39, 0xFFFF,                              0));
      check_csr_wrrd("vsatp Sv39 PPN width limits",       CSR_VSATP, make_apt(MODE_SV39,      0, origin_ppn), make_apt(MODE_SV39,      0, sv39x4_limited_right_vsatp_ppn));

      check_csr_wrrd("vsatp Sv48",                        CSR_VSATP, make_apt(MODE_SV48,      0,          0), make_apt(MODE_SV48,      0,                              0));
      check_csr_wrrd("vsatp Sv48 ASID",                   CSR_VSATP, make_apt(MODE_SV48, 0xFFFF,          0), make_apt(MODE_SV48, 0xFFFF,                              0));
      check_csr_wrrd("vsatp Sv48 PPN width limits",       CSR_VSATP, make_apt(MODE_SV48,      0, origin_ppn), make_apt(MODE_SV48,      0, sv48x4_limited_right_vsatp_ppn));

      CSRW(CSR_VSATP, make_apt(MODE_SV39, 0xFFFF, origin_ppn));
      check_csr_wrrd("vsatp mode(ill) others(WARL) in MODE_M", CSR_VSATP, make_apt(MODE_ILL, 0, ppn_dead_beef), make_apt(MODE_SV39, 0xFFFF, sv39x4_limited_vsatp_ppn_dead_beef));
    }
  }

  DETAIL("Test vsatp when hgatp.MODE=Sv48x4")
  if (true) {
    goto_priv(PRIV_M);
    // set hgatp.mode = Sv39
    CSRW(CSR_HGATP, make_apt(MODE_SV48x4, 0, 0));

    {
      CSRW(CSR_SATP, make_apt(MODE_BARE, 0, 0));
      check_csr_wrrd("vsatp Bare, when satp Bare",        CSR_VSATP, make_apt(MODE_BARE, 0, 0), make_apt(MODE_BARE, 0, 0));
      CSRW(CSR_SATP, make_apt(MODE_SV39, 0, 0));
      check_csr_wrrd("vsatp Bare, when satp Sv39",        CSR_VSATP, make_apt(MODE_BARE, 0, 0), make_apt(MODE_BARE, 0, 0));
      CSRW(CSR_SATP, make_apt(MODE_SV48, 0, 0));
      check_csr_wrrd("vsatp Bare, when satp Sv48",        CSR_VSATP, make_apt(MODE_BARE, 0, 0), make_apt(MODE_BARE, 0, 0));
    }

    {
      CSRW(CSR_SATP, make_apt(MODE_BARE, 0, 0));
      check_csr_wrrd("vsatp Sv39",                        CSR_VSATP, make_apt(MODE_SV39,      0,          0), make_apt(MODE_SV39,      0,                              0));
      check_csr_wrrd("vsatp Sv39 ASID",                   CSR_VSATP, make_apt(MODE_SV39, 0xFFFF,          0), make_apt(MODE_SV39, 0xFFFF,                              0));
      check_csr_wrrd("vsatp Sv39 PPN width limits",       CSR_VSATP, make_apt(MODE_SV39,      0, origin_ppn), make_apt(MODE_SV39,      0, sv39x4_limited_right_vsatp_ppn));

      check_csr_wrrd("vsatp Sv48",                        CSR_VSATP, make_apt(MODE_SV48,      0,          0), make_apt(MODE_SV48,      0,                              0));
      check_csr_wrrd("vsatp Sv48 ASID",                   CSR_VSATP, make_apt(MODE_SV48, 0xFFFF,          0), make_apt(MODE_SV48, 0xFFFF,                              0));
      check_csr_wrrd("vsatp Sv48 PPN width limits",       CSR_VSATP, make_apt(MODE_SV48,      0, origin_ppn), make_apt(MODE_SV48,      0, sv48x4_limited_right_vsatp_ppn));

      CSRW(CSR_VSATP, make_apt(MODE_SV39, 0xFFFF, origin_ppn));
      check_csr_wrrd("vsatp mode(ill) others(WARL) in MODE_M", CSR_VSATP, make_apt(MODE_ILL, 0, ppn_dead_beef), make_apt(MODE_SV39, 0xFFFF, sv39x4_limited_vsatp_ppn_dead_beef));
    }
  }

  TEST_END();
}
