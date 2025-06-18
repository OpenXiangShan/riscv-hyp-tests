#include <page_tables.h>
#include <rvh_test.h>
#define USE_CYCLE_COUNTER 1
// #define USE_TIME_COUNTER 1

#define PERF_4B 1
// #define PERF_1B 1
#define ReadReg32(addr) (*(volatile uint32_t *)(addr))
#define WriteReg32(addr, data) (ReadReg32(addr) = (data))
#define ClearBit32(addr, width, offset) ReadReg32(addr) & ~(((1 << (width))-1) << (offset))
#define WriteBit32(addr, data, width, offset) (ReadReg32(addr) = (ClearBit32(addr, width, offset) | ((data) << (offset))))

static inline void touchread(uintptr_t addr){
  asm volatile("" ::: "memory");
  volatile uint64_t x = *(volatile uint64_t *)addr;
}

static inline void touchwrite(uintptr_t addr, uint64_t val){
  asm volatile("" ::: "memory");
  *((volatile uint64_t*) addr) = val;
}

static inline uint64_t myClock(){
  uint64_t res = 0;
#if USE_CYCLE_COUNTER
  asm volatile(
    "fence;"
    "csrr %0, cycle;"
    "fence;"
    : "=r"(res) : :
  );
#elif USE_TIME_COUNTER
  asm volatile(
    "fence;"
    "csrr %0, time;"
    "fence;"
    : "=r"(res) : :
  );
#endif
  return res;
}

static inline uint64_t read4KBby4BAndPrint(uintptr_t addr){
  printf("addr:0x%lx\n", addr);
  volatile unsigned long res, start, end;
  start = myClock();
  uint64_t val;
#ifdef PERF_4B
  for(int i = 0; i < 1000; i++){
    read32(addr);
    addr += 4;
  }
#elif PERF_1B
  for(int i = 0; i < 4096; i++){
    read8(addr);
    addr += 1;
  }
#endif
  end = myClock();
  printf("start: %lu, end: %lu, ticks: %lu\n", start, end, res = (end - start));
  return res;
}


bool test_pbmte() {
  TEST_START();

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
  CSRS(CSR_HEDELEG, 1 << CAUSE_LPF);
  CSRS(CSR_HENVCFG, HENVCFG_PBMTE);
  hfence_vvma();
  goto_priv(PRIV_VS);

  printf("\n");
  INFO("test (menvcfg.pbmte, PBMT) = (0, 0)")
  touchread(vs_page_base(VSRWX_GURWX));

  INFO("test (menvcfg.pbmte, PBMT) = (0, 1)");
  TEST_SETUP_EXCEPT();
  touchread(vs_page_base(VSRWX_GURWXP));
  TEST_ASSERT("load page fault triggered", excpt.triggered == true && excpt.cause == CAUSE_LGPF)

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
  CSRS(CSR_HENVCFG, HENVCFG_PBMTE);
  hfence_vvma();
  goto_priv(PRIV_VS);
  touchread(vs_page_base(VSRWXP_GURWXP));

  TEST_END();
}

bool test_pbmt() {
  TEST_START();
  INFO("\ntest pbmt (insufficient)")
  
  // close all delegations of exceptions
  CSRW(CSR_MEDELEG, 0);
  CSRW(CSR_HEDELEG, 0);

  // Setup hyp page_tables.
  goto_priv(PRIV_HS);
  hspt_init();
  hpt_init();

  // Setup guest page tables.
  goto_priv(PRIV_VS);
  vspt_init();

  uintptr_t vaddr = vs_page_base(VSRWXP_GURWXP);
  uint64_t value = 0xdeadbeef;

  INFO("NC LD Access")
  goto_priv(PRIV_M);
  CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
  hfence_gvma();
  CSRS(CSR_HENVCFG, HENVCFG_PBMTE);
  hfence_vvma();
  goto_priv(PRIV_VS);
  printf("vaddr = 0x%lx\n", vaddr);
  write64(vaddr, value);

  TEST_END();
}

bool test_pbmt_simple() {
  TEST_START();
  
  // Close all delegations of exceptions
  CSRW(CSR_MEDELEG, 0);
  CSRW(CSR_HEDELEG, 0);

  // Setup hyp page_tables.
  goto_priv(PRIV_HS);
  hspt_init();
  hpt_init();

  // Setup guest page tables.
  goto_priv(PRIV_VS);
  vspt_init();

  // Set PBMTE
  goto_priv(PRIV_M);
  CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
  hfence_gvma();
  CSRS(CSR_HENVCFG, HENVCFG_PBMTE);
  hfence_vvma();
  goto_priv(PRIV_VS);

  uintptr_t vaddr = vs_page_base(VSRWXP_GURWXP);
  uint64_t val = 0xdeadbeef;
  // printf("\n");
  // printf("vaddr = 0x%lx\n", vaddr);
  write64(vaddr, val);
  // printf("read = 0x%lx\n", read64(vaddr));
  TEST_SETUP_EXCEPT();
  bool check = read64(vaddr) == val;  
  // printf("%d\n", check);
  TEST_ASSERT("NC ST and LD is successed", excpt.triggered == false && check);

  TEST_END();
}

bool test_pbmt_forward() {
  TEST_START();
  
  // Close all delegations of exceptions
  CSRW(CSR_MEDELEG, 0);
  CSRW(CSR_HEDELEG, 0);

  // Setup hyp page_tables.
  goto_priv(PRIV_HS);
  hspt_init();
  hpt_init();

  // Setup guest page tables.
  goto_priv(PRIV_VS);
  vspt_init();

  // Set PBMTE
  goto_priv(PRIV_M);
  CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
  hfence_gvma();
  CSRS(CSR_HENVCFG, HENVCFG_PBMTE);
  hfence_vvma();
  goto_priv(PRIV_VS);

  
  uintptr_t vaddr = vs_page_base(VSRWXP_GURWXP);
  uint64_t val = 0xdeadbeef;
  
  uintptr_t vaddrs[5];
  uint64_t vals[5];
  vaddrs[0] = vaddr;
  for(int i = 1; i < 5; ++i) {
    vaddrs[i] = vaddrs[i-1] + 8;
  }
  
  for(int i = 1; i < 5; ++i) {
    *(uint64_t*) vaddrs[i] = val >> (i * 4);
  }

  for(int i = 1; i < 5; ++i) {
    vals[i] = *(volatile uint64_t*) vaddrs[i];
  }

  // printf("\n");
  // printf("vaddr, val\n");
  // for(int i = 1; i < 5; ++i) {
  //   printf("%d: 0x%lx, 0x%lx\n", i, vaddrs[i], vals[i]);
  // }
  TEST_SETUP_EXCEPT();
  TEST_ASSERT("vals[1] should be 0xdeadbee.", vals[1]==0xdeadbee);
  TEST_SETUP_EXCEPT();
  TEST_ASSERT("vals[2] should be 0xdeadbe.", vals[2]==0xdeadbe);
  TEST_SETUP_EXCEPT();
  TEST_ASSERT("vals[3] should be 0xdeadb.", vals[3]==0xdeadb);
  TEST_SETUP_EXCEPT();
  TEST_ASSERT("vals[4] should be 0xdead.", vals[4]==0xdead);

  TEST_END();
}

bool test_pbmt_stld_violate() {
  TEST_START();
  // Close all delegations of exceptions
  CSRW(CSR_MEDELEG, 0);
  CSRW(CSR_HEDELEG, 0);

  // Setup hyp page_tables.
  goto_priv(PRIV_HS);
  hspt_init();
  hpt_init();

  // Setup guest page tables.
  goto_priv(PRIV_VS);
  vspt_init();

  // Set PBMTE
  goto_priv(PRIV_M);
  CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
  hfence_gvma();
  CSRS(CSR_HENVCFG, HENVCFG_PBMTE);
  hfence_vvma();
  goto_priv(PRIV_VS);

  // Basic settings
  uintptr_t vaddr = vs_page_base(VSRWXP_GURWXP);
  uint64_t val = 0xdeadbeef;
  // asm volatile("" ::: "memory");
  // asm volatile("fence");
  uintptr_t vaddr1, vaddr2;
  uint64_t val1, val2;
  vaddr2 = 0x10006c008;
  *(uint64_t*) vaddr = vaddr2;

  // stld violate
  asm (
    // address: vaddr1 = *(uint64_t*) vaddr;
    // "slli %1, %3, 1;"
    // "srli %1, %1, 1;"
    // "addi %1, %1, -16;"
    // "addi %1, %1, 24;"
    
    // "ld %1, 0(%3);"
    "ld t0, 0(%3);"
    "ld t1, 0(%3);"
    "ld t2, 0(%3);"
    "add %1, t0, t1;"
    "sub %1, %1, t2;"

    // data: val1 = (val << 32) + ((vaddr1 << 32 >> 32)- 8) + 8;
    "slli t1, %1, 32;"
    "slli t0, %2, 32;"
    "srli t1, t1, 32;"
    "addi t1, t1, -8;"
    "add %0, t0, t1;"
    "addi %0, %0, 8;"
    : "=r"(val1), "=r"(vaddr1)
    : "r"(val), "r"(vaddr)
    : "t0", "t1", "t2"
  );
  *(uint64_t*) vaddr1 = val1;
  val2 = *(uint64_t*) vaddr2;
  
  // Output
  // printf("\n");
  // printf("%d: 0x%lx, 0x%lx\n", 1, vaddr1, val1);
  // printf("%d: 0x%lx, 0x%lx\n", 2, vaddr2, val2);
  TEST_SETUP_EXCEPT();
  TEST_ASSERT("There should be the same vaddr.", vaddr1==vaddr2);
  TEST_SETUP_EXCEPT();
  TEST_ASSERT("There should be the same data.", val1==val2);
  
  TEST_END();
}

bool test_pbmt_ldld_violate() {
  TEST_START();
  // Close all delegations of exceptions
  CSRW(CSR_MEDELEG, 0);
  CSRW(CSR_HEDELEG, 0);

  // Setup hyp page_tables.
  goto_priv(PRIV_HS);
  hspt_init();
  hpt_init();

  // Setup guest page tables.
  goto_priv(PRIV_VS);
  vspt_init();

  // Set PBMTE
  goto_priv(PRIV_M);
  CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
  hfence_gvma();
  CSRS(CSR_HENVCFG, HENVCFG_PBMTE);
  hfence_vvma();
  goto_priv(PRIV_VS);

  // Basic settings
  uintptr_t vaddr = vs_page_base(VSRWXP_GURWXP);
  uint64_t val = 0xdeadbeef;
  uintptr_t vaddr1, vaddr2;
  uint64_t val1, val2;
  vaddr2 = vaddr + 8;

  // ldld violate
  asm volatile(
    // *(uint64_t*) vaddr2 = val;
    "sd %3, 0(%2);"
    // *(uint64_t*) vaddr = vaddr2;
    "sd %2, 0(%1);"
    // address: vaddr1 = *(uint64_t*) vaddr;
    
    "ld t0, 0(%1);"
    "ld t1, 0(%1);"
    "ld t2, 0(%1);"
    "add %0, t0, t1;"
    "sub %0, %0, t2;"
    : "=r"(vaddr1)
    : "r"(vaddr), "r"(vaddr2), "r"(val)
    : "t0", "t1", "t2"
  );
  val1 = *(uint64_t*) vaddr1;
  val2 = *(uint64_t*) vaddr2;

  // Output
  // printf("\n");
  // printf("%d: 0x%lx, 0x%lx\n", 1, vaddr1, val1);
  // printf("%d: 0x%lx, 0x%lx\n", 2, vaddr2, val2);
  TEST_SETUP_EXCEPT();
  TEST_ASSERT("There should be the same vaddr.", vaddr1==vaddr2);
  TEST_SETUP_EXCEPT();
  TEST_ASSERT("There should be the same data.", val1==val2);

  TEST_END();
}

static inline bool ByteWriteRead(uintptr_t vaddr){
  bool check = true;
  uint64_t st_data = 0xdeadbeef;
  WriteBit32(vaddr, st_data, 32, 0);
  uint64_t ld_data = ReadReg32(vaddr);
  check = check && (ld_data == st_data);
  printf("4-byte wr: vaddr = 0x%lx, st_data = 0x%lx, ld_data = 0x%lx\n", vaddr, st_data, ld_data);

  st_data = 0xdeaddcba;
  WriteBit32(vaddr, 0xa, 4, 0);
  WriteBit32(vaddr, 0xb, 4, 4);
  WriteBit32(vaddr, 0xc, 4, 8);
  WriteBit32(vaddr, 0xd, 4, 12);
  ld_data = ReadReg32(vaddr);
  check = check && (ld_data == st_data);
  printf("1-byte wr: vaddr = 0x%lx, correct = 0x%lx, ld_data = 0x%lx\n", vaddr, st_data, ld_data);

  for(unsigned int i = 0; i < 60; i = i + 4) {
    WriteReg32(vaddr + i, 0xc000 + i);
  }
  
  for(unsigned int i = 0; i < 60; i = i + 4) {
    ld_data = ReadReg32(vaddr + i);
    check = check && (ld_data == (0xc000 + i));
    // printf("vaddr + %d = 0x%lx\n", i, ld_data);
  }

  return check;
}

bool test_pbmt_byte_write_read(){
  TEST_START();
  // Close all delegations of exceptions
  CSRW(CSR_MEDELEG, 0);
  CSRW(CSR_HEDELEG, 0);

  // Setup hyp page_tables.
  goto_priv(PRIV_HS);
  hspt_init();
  hpt_init();

  // Setup guest page tables.
  goto_priv(PRIV_VS);
  vspt_init();

  // Set PBMTE
  goto_priv(PRIV_M);
  CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
  hfence_gvma();
  CSRS(CSR_HENVCFG, HENVCFG_PBMTE);
  hfence_vvma();
  goto_priv(PRIV_VS);

  bool check;
  uintptr_t nc_vaddr = vs_page_base(VSRWXP_GURWXP);
  printf("\nSvpbmt NC test...\n");
  check = ByteWriteRead(nc_vaddr);
  TEST_SETUP_EXCEPT();
  TEST_ASSERT("Something is wrong.", check);

  goto_priv(PRIV_M);
  CSRS(0x5C3, 1<<7);
  printf("\nSvpbmt NC OUTSTANDING test...\n");
  printf("smblockctl = 0x%lx\n", CSRR(0x5C3));
  goto_priv(PRIV_VS);
  check = ByteWriteRead(nc_vaddr);
  TEST_SETUP_EXCEPT();
  TEST_ASSERT("Something is wrong.", check);

  TEST_END();
}

bool test_pbmt_perf(){
  TEST_START();
  // Close all delegations of exceptions
  CSRW(CSR_MEDELEG, 0);
  CSRW(CSR_HEDELEG, 0);

  // Setup hyp page_tables.
  goto_priv(PRIV_HS);
  hspt_init();
  hpt_init();

  // Setup guest page tables.
  goto_priv(PRIV_VS);
  vspt_init();

  // Set PBMTE
  goto_priv(PRIV_M);
  CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
#if USE_CYCLE_COUNTER
  CSRS(CSR_MCOUNTEREN, HCOUNTEREN_CY);
  CSRW(CSR_MCYCLE, 0x0);
#elif USE_TIME_COUNTER
  CSRS(CSR_MCOUNTEREN, HCOUNTEREN_TM);
#endif
  hfence_gvma();
  // goto_priv(PRIV_HS);
  CSRS(CSR_HENVCFG, HENVCFG_PBMTE);
#ifdef USE_CYCLE_COUNTER
  CSRS(CSR_HCOUNTEREN, HCOUNTEREN_CY);
#elif USE_TIME_COUNTER
  CSRS(CSR_HCOUNTEREN, HCOUNTEREN_TM);
#endif
  hfence_vvma();
  goto_priv(PRIV_VS);
#ifdef USE_CYCLE_COUNTER
  CSRW(sscratch, 1);
#endif

  uint64_t val;
  unsigned long io_time, nc_time, nc_ot_time, pma_time;
#ifdef PERF_4B
  printf("\nTEST: read 4 Bytes 1000 times\n");
#elif PERF_1B
  printf("\nTEST: read 1 Byte 4096 times\n");
#endif

  uintptr_t io_vaddr = vs_page_base(VSRWXP2_GURWXP2);
  printf("\nSvpbmt IO test...\n");
  io_time = read4KBby4BAndPrint(io_vaddr);

  uintptr_t nc_vaddr = vs_page_base(VSRWXP_GURWXP);
  printf("\nSvpbmt NC test...\n");
  nc_time = read4KBby4BAndPrint(nc_vaddr);

  goto_priv(PRIV_M);
  CSRS(0x5C3, 1<<7);
  printf("\nSvpbmt NC OUTSTANDING test...\n");
  printf("smblockctl = 0x%lx\n", CSRR(0x5C3));
  goto_priv(PRIV_VS);
  nc_ot_time = read4KBby4BAndPrint(nc_vaddr);
  
  uintptr_t pma_vaddr = vs_page_base(VSRWX_GURWX);
  printf("\nSvpbmt PMA test...\n");
  pma_time = read4KBby4BAndPrint(pma_vaddr);

  bool time_order = io_time > nc_time && nc_time > nc_ot_time && nc_ot_time > pma_time;
  TEST_SETUP_EXCEPT();
  TEST_ASSERT("Time order need to be 'IO > NC > NC OT > PMA'", time_order);
  
  TEST_END();
}