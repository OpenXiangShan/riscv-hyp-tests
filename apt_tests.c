#include <rvh_test.h>
#include <page_tables.h>

#define PaddrWidth  (36)
#define InPageWidth (12)
#define VmidWidth   (14)
#define AsidWidth   (16)
#define Sv39x4Width (41)

#define MODE_BARE 0
#define MODE_SV39 (0x8ULL << 60)
#define MODE_ILL  (1ULL << 60)

const uint64_t origin_ppn = 0xfffffffffff;
const uint64_t BARE_MASK = BIT_MASK(0, PaddrWidth - InPageWidth);
const uint64_t SV39X4_MASK = BIT_MASK(0, Sv39x4Width - InPageWidth);
const uint64_t paddr_limited_right_satp_ppn   = origin_ppn & BARE_MASK;
const uint64_t paddr_limited_right_hgatp_ppn  = origin_ppn & BARE_MASK & ~(uint64_t)(0x3);
const uint64_t bare_limited_right_vsatp_ppn   = origin_ppn & BARE_MASK;
const uint64_t sv39x4_limited_right_vsatp_ppn = origin_ppn & SV39X4_MASK;
const uint64_t ppn_dead_beef = 0xdeadbeef;
const uint64_t bare_limited_vsatp_ppn_dead_beef = ppn_dead_beef & BARE_MASK;
const uint64_t sv39x4_limited_vsatp_ppn_dead_beef = ppn_dead_beef & SV39X4_MASK;

bool apt_tests() {

  TEST_START();

  goto_priv(PRIV_M);

  CSRW(CSR_MEDELEG, 0);

  DETAIL("Test hgatp");
  if (true) {
    {
      CSRW(CSR_SATP,  MODE_SV39);
      CSRW(CSR_VSATP, MODE_SV39);

      check_csr_wrrd("hgatp Bare",                           CSR_HGATP,                             0, 0);
      check_csr_wrrd("hgatp Bare VMID",                      CSR_HGATP,         0 | (0x3FFFULL << 44), (0x3FFFULL << 44));
      check_csr_wrrd("hgatp Bare PPN",                       CSR_HGATP,         0 |        origin_ppn, paddr_limited_right_hgatp_ppn);
    }

    {
      CSRW(CSR_SATP,  MODE_BARE);
      CSRW(CSR_VSATP, MODE_BARE);
      check_csr_wrrd("hgatp Sv39x4",                         CSR_HGATP,                  0x8ULL << 60, 0x8ULL << 60);
      check_csr_wrrd("hgatp Sv39x4 VMID",                    CSR_HGATP, MODE_SV39 | (0x3FFFULL << 44), (0x8ULL << 60) | (0x3FFFULL << 44));
      check_csr_wrrd("hgatp Sv39x4 PPN width limits",        CSR_HGATP, MODE_SV39 |        origin_ppn, (0x8ULL << 60) | paddr_limited_right_hgatp_ppn);
      check_csr_wrrd("hgatp mode(illegal)",                  CSR_HGATP, MODE_ILL                     , 0);
      check_csr_wrrd("hgatp mode(illegal) VMID",             CSR_HGATP, MODE_ILL  | (0x3FFFULL << 44), (0x3FFFULL << 44));
      check_csr_wrrd("hgatp mode(illegal) PPN width limits", CSR_HGATP, MODE_ILL  |        origin_ppn, paddr_limited_right_hgatp_ppn);
    }
  }

  DETAIL("Test satp");
  if (true) {
    {
      CSRW(CSR_HGATP, MODE_SV39);
      CSRW(CSR_VSATP, MODE_SV39);
      check_csr_wrrd("satp Bare",                        CSR_SATP,                                  0, 0);
    }
    {
      CSRW(CSR_HGATP, MODE_BARE);
      CSRW(CSR_VSATP, MODE_BARE);
      check_csr_wrrd("satp Sv39",                        CSR_SATP,                       0x8ULL << 60, 0x8ULL << 60);
      check_csr_wrrd("satp Sv39 ASID",                   CSR_SATP, (0x8ULL << 60) | (0xFFFFULL << 44), (0x8ULL << 60) | (0xFFFFULL << 44));
      check_csr_wrrd("satp Sv39 PPN width limits",       CSR_SATP, (0x8ULL << 60) |        origin_ppn, (0x8ULL << 60) | paddr_limited_right_satp_ppn);

      CSRW(CSR_SATP, MODE_SV39 | origin_ppn);
      check_csr_wrrd("satp mode(illegal) others(keep)",  CSR_SATP,  0x1ULL << 60                     , (0x8ULL << 60) | paddr_limited_right_satp_ppn);
    }
  }

  DETAIL("Test vsatp when hgatp.MODE=Bare");
  if (true) {
    // set hgatp.mode = Bare
    CSRW(CSR_HGATP, MODE_BARE);
    {
      CSRW(CSR_SATP, MODE_SV39);
      check_csr_wrrd("vsatp Bare",                        CSR_VSATP,                                  0, 0);
    }
    {
      CSRW(CSR_SATP, MODE_BARE);
      check_csr_wrrd("vsatp Sv39",                        CSR_VSATP,                       0x8ULL << 60, 0x8ULL << 60);
      check_csr_wrrd("vsatp Sv39 ASID",                   CSR_VSATP, (0x8ULL << 60) | (0xFFFFULL << 44), (0x8ULL << 60) | (0xFFFFULL << 44));
      check_csr_wrrd("vsatp Sv39 PPN width limits",       CSR_VSATP, (0x8ULL << 60) |        origin_ppn, (0x8ULL << 60) | bare_limited_right_vsatp_ppn);

      CSRW(CSR_VSATP, MODE_SV39 | origin_ppn);
      check_csr_wrrd("vsatp mode(illegal) others(WARL) in MODE_M",  CSR_VSATP,  0x1ULL << 60  |     ppn_dead_beef, (0x8ULL << 60) | bare_limited_vsatp_ppn_dead_beef);

      CSRW(CSR_VSATP, MODE_BARE);

      DEBUG("hgatp: %lx", CSRR(CSR_HGATP));
      DEBUG("vsatp: %lx", CSRR(CSR_VSATP));
      DEBUG(" satp: %lx", CSRR(CSR_SATP));
      DEBUG("medeleg: %lx", CSRR(CSR_MEDELEG));

      goto_priv(PRIV_VS);

      CSRW(CSR_SATP, 0);
      DEBUG("vsatp: %lx", CSRR(CSR_SATP));
      check_csr_wrrd("vsatp mode(illegal) others(keep) in MODE_VS",  CSR_SATP,   0x1ULL << 60  |              ppn_dead_beef, 0);
    }
  }

  DETAIL("Test vsatp when hgatp.MODE=Sv39x4")
  if (true) {
    goto_priv(PRIV_M);
    // set hgatp.mode = Sv39
    CSRW(CSR_HGATP, MODE_SV39);

    {
      CSRW(CSR_SATP, MODE_SV39);
      DEBUG("hgatp=%lx", CSRR(CSR_HGATP));
      check_csr_wrrd("vsatp Bare",                        CSR_VSATP,                                  0, 0);
    }

    {
      CSRW(CSR_SATP, MODE_BARE);
      check_csr_wrrd("vsatp Sv39",                        CSR_VSATP,                       0x8ULL << 60, 0x8ULL << 60);
      check_csr_wrrd("vsatp Sv39 ASID",                   CSR_VSATP, (0x8ULL << 60) | (0xFFFFULL << 44), (0x8ULL << 60) | (0xFFFFULL << 44));
      check_csr_wrrd("vsatp Sv39 PPN width limits",       CSR_VSATP, (0x8ULL << 60) |        origin_ppn, (0x8ULL << 60) | sv39x4_limited_right_vsatp_ppn);

      CSRW(CSR_VSATP, (0x8ULL << 60) | origin_ppn);
      check_csr_wrrd("vsatp mode(illegal) others(WARL) in MODE_M",  CSR_VSATP,  0x1ULL << 60  |     ppn_dead_beef, (0x8ULL << 60) | sv39x4_limited_vsatp_ppn_dead_beef);
    }
  }

  TEST_END();
}
