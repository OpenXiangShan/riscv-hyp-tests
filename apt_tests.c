#include <rvh_test.h>
#include <page_tables.h> 

#define PaddrWidth  (36)
#define InPageWidth (12)
#define VmidWidth   (14)
#define AsidWidth   (16)
#define Sv39x4Width (41)

bool apt_tests() {

    TEST_START();

    uint64_t origin_ppn = 0xffffffffff;
    uint64_t paddr_limited_right_satp_ppn  = origin_ppn & BIT_MASK(0, PaddrWidth - InPageWidth);
    uint64_t paddr_limited_right_hgatp_ppn = origin_ppn & BIT_MASK(0, PaddrWidth - InPageWidth) & ~(uint64_t)(0x3);
    uint64_t bare_limited_right_vsatp_ppn  = origin_ppn & BIT_MASK(0, PaddrWidth - InPageWidth);
    uint64_t sv39x4_limited_right_vsatp_ppn = origin_ppn & BIT_MASK(0, Sv39x4Width - InPageWidth);

    DETAIL("Test hgatp")
    check_csr_wrrd("hgatp Bare",                           CSR_HGATP,                                  0, 0);
    check_csr_wrrd("hgatp Bare other fields all zero",     CSR_HGATP,                    BIT_MASK(0, 60), 0);
    check_csr_wrrd("hgatp Sv39x4",                         CSR_HGATP,                       0x8ULL << 60, 0x8ULL << 60);
    check_csr_wrrd("hgatp Sv39x4 VMID",                    CSR_HGATP, (0x8ULL << 60) | (0x3FFFULL << 44), (0x8ULL << 60) | (0x3FFFULL << 44));
    check_csr_wrrd("hgatp Sv39x4 PPN width limits",        CSR_HGATP, (0x8ULL << 60) |        origin_ppn, (0x8ULL << 60) | paddr_limited_right_hgatp_ppn);
    check_csr_wrrd("hgatp mode(illegal)",                  CSR_HGATP,  0x1ULL << 60                     , 0);
    check_csr_wrrd("hgatp mode(illegal) VMID",             CSR_HGATP, (0x1ULL << 60) | (0x3FFFULL << 44), (0x3FFFULL << 44));
    check_csr_wrrd("hgatp mode(illegal) PPN width limits", CSR_HGATP, (0x1ULL << 60) |        origin_ppn, paddr_limited_right_hgatp_ppn);

    DETAIL("Test satp")
    check_csr_wrrd("satp Bare",                        CSR_SATP,                                  0, 0);
    check_csr_wrrd("satp Sv39",                        CSR_SATP,                       0x8ULL << 60, 0x8ULL << 60);
    check_csr_wrrd("satp Sv39 ASID",                   CSR_SATP, (0x8ULL << 60) | (0xFFFFULL << 44), (0x8ULL << 60) | (0xFFFFULL << 44));
    check_csr_wrrd("satp Sv39 PPN width limits",       CSR_SATP, (0x8ULL << 60) |        origin_ppn, (0x8ULL << 60) | paddr_limited_right_satp_ppn);

    CSRW(CSR_SATP, (0x8ULL << 60) | origin_ppn);
    check_csr_wrrd("satp mode(illegal) others(keep)",  CSR_SATP,  0x1ULL << 60                     , (0x8ULL << 60) | paddr_limited_right_satp_ppn);

    // set hgatp.mode = Bare
    DETAIL("Test vsatp when hgatp.MODE=Bare")
    CSRW(CSR_HGATP, 0);
    check_csr_wrrd("vsatp Bare",                        CSR_VSATP,                                  0, 0);
    check_csr_wrrd("vsatp Sv39",                        CSR_VSATP,                       0x8ULL << 60, 0x8ULL << 60);
    check_csr_wrrd("vsatp Sv39 ASID",                   CSR_VSATP, (0x8ULL << 60) | (0xFFFFULL << 44), (0x8ULL << 60) | (0xFFFFULL << 44));
    check_csr_wrrd("vsatp Sv39 PPN width limits",       CSR_VSATP, (0x8ULL << 60) |        origin_ppn, (0x8ULL << 60) | bare_limited_right_vsatp_ppn);
    CSRW(CSR_VSATP, (0x8ULL << 60) | origin_ppn);
    check_csr_wrrd("vsatp mode(illegal) others(keep)",  CSR_VSATP,  0x1ULL << 60                     , (0x8ULL << 60) | bare_limited_right_vsatp_ppn);

    DETAIL("Test vsatp when hgatp.MODE=Sv39x4")
    CSRW(CSR_HGATP, (0x8ULL << 60));
    CSRW(CSR_VSATP, 0);
    DEBUG("hgatp=%lx\n", CSRR(CSR_HGATP));
    DEBUG("vsatp=%lx\n", CSRR(CSR_VSATP));
    check_csr_wrrd("vsatp Bare",                        CSR_VSATP,                                  0, 0);
    check_csr_wrrd("vsatp Sv39",                        CSR_VSATP,                       0x8ULL << 60, 0x8ULL << 60);
    check_csr_wrrd("vsatp Sv39 ASID",                   CSR_VSATP, (0x8ULL << 60) | (0xFFFFULL << 44), (0x8ULL << 60) | (0xFFFFULL << 44));
    check_csr_wrrd("vsatp Sv39 PPN width limits",       CSR_VSATP, (0x8ULL << 60) |        origin_ppn, (0x8ULL << 60) | sv39x4_limited_right_vsatp_ppn);
    CSRW(CSR_VSATP, (0x8ULL << 60) | origin_ppn);
    check_csr_wrrd("vsatp mode(illegal) others(keep)",  CSR_VSATP,  0x1ULL << 60                     , (0x8ULL << 60) | sv39x4_limited_right_vsatp_ppn);


    TEST_END(); 
}
