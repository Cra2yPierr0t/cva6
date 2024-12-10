typedef unsigned long uint64_t;

#define M_MODE 0x11
#define HS_MODE 0x01
#define VS_MODE 0x01
#define VU_MODE 0x00

#define V_MODE 0x1

#define MSTATUS_MPP 11
#define HSTATUS_SPV 7
#define SSTATUS_SPP 8

#define PTE_V 0x1
#define PTE_R 0x2
#define PTE_W 0x4
#define PTE_X 0x8
#define PTE_U 0x10
#define PTE_A 0x40
#define PTE_D 0x80

#define L3_PAGETABLE 0x80010000
#define L2_PAGETABLE 0x80011000
#define L1_PAGETABLE 0x80012000

#define VM_L3_PAGETABLE 0x80013000
#define VM_L2_PAGETABLE 0x80014000
#define VM_L1_PAGETABLE 0x80015000

#define MMU_EN 0x8000000000000000

void init_pmp() {
  //S-mode can access 0x00000000 ~ 0xffffffff range
  asm volatile("li    t0      , 0x0000001f");
  asm volatile("csrw  pmpcfg0 , t0");
  asm volatile("li    t0      , 0xffffffff");
  asm volatile("csrw  pmpaddr0, t0");
  return;
}

void enter_hsmode() {
  uint64_t mstatus;
  asm volatile("csrr  %0      , mstatus" : "=r" (mstatus));
  mstatus = mstatus | (HS_MODE << MSTATUS_MPP);
  asm volatile("csrw  mstatus , %0" : : "r" (mstatus));
  asm volatile("la    t0      , _now_hsmode");
  asm volatile("csrw  mepc    , t0");
  asm volatile("mret");
  asm volatile("_now_hsmode:"); // here is in HS-mode
  return;
}

void enter_vsmode() {
  uint64_t hstatus, sstatus;
  asm volatile("csrr  %0      , hstatus" : "=r" (hstatus));
  asm volatile("csrr  %0      , sstatus" : "=r" (sstatus));
  hstatus = hstatus | (V_MODE << HSTATUS_SPV);
  sstatus = sstatus | (VS_MODE << SSTATUS_SPP);
  asm volatile("csrw  hstatus , %0" : : "r" (hstatus));
  asm volatile("csrw  sstatus , %0" : : "r" (sstatus));
  asm volatile("la    t0      , _now_vsmode");
  asm volatile("csrw  sepc    , t0");
  asm volatile("sret");
  asm volatile("_now_vsmode:"); // here is in VS-mode
  return;
}

void init_g_stage_pagetable() {
  uint64_t hgatp;
  uint64_t *pte;

  // Direct Mapping 0x80000000 ~ 0x80030000
  hgatp = L3_PAGETABLE >> 12;

  pte = L3_PAGETABLE | ((0x80000000 >> 30) << 3);
  *pte = ((L2_PAGETABLE >> 12) << 10) | PTE_V;

  pte = L2_PAGETABLE | ((0x80000000 & 0x3fe00000) >> 21) << 3;
  *pte = ((L1_PAGETABLE >> 12) << 10) | PTE_V;

  for(int i = 0; i < 0x31; i++) {
    pte = L1_PAGETABLE | (((0x80000000 + (i * 0x1000)) & 0x1ff000) >> 12) << 3;
    *pte = (((0x80000000 + (i * 0x1000)) >> 12) << 10) | PTE_V | PTE_R | PTE_W | PTE_X | PTE_U | PTE_A | PTE_D;
  }

  // activate MMU
  hgatp |= MMU_EN;
  asm volatile("csrw hgatp, %0" : : "r" (hgatp));
  asm volatile("sfence.vma zero, zero");

  return;
}

void init_vs_stage_pagetable() {
  uint64_t vsatp;
  uint64_t *pte;

  // Direct Mapping 0x80000000 ~ 0x80030000
  vsatp = VM_L3_PAGETABLE >> 12;

  pte = VM_L3_PAGETABLE | ((0x80000000 >> 30) << 3);
  *pte = ((VM_L2_PAGETABLE >> 12) << 10) | PTE_V;

  pte = VM_L2_PAGETABLE | ((0x80000000 & 0x3fe00000) >> 21) << 3;
  *pte = ((VM_L1_PAGETABLE >> 12) << 10) | PTE_V;

  for(int i = 0; i < 0x31; i++) {
    pte = VM_L1_PAGETABLE | (((0x80000000 + (i * 0x1000)) & 0x1ff000) >> 12) << 3;
    *pte = (((0x80000000 + (i * 0x1000)) >> 12) << 10) | PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
  }

  vsatp |= MMU_EN;
  asm volatile("csrw satp, %0" : : "r" (vsatp));
  asm volatile("sfence.vma zero, zero");

  return;
}


int main() {
  init_pmp();

  enter_hsmode();

  init_g_stage_pagetable();

  enter_vsmode();

  init_vs_stage_pagetable();

  uint64_t *x = (uint64_t *)0x80023000; // katte ni tsukau
  for(int i = 0; i < 6; i++) { // takusan page tsukau
    x[i*512] = i * 7;
  }

  return 0;
}
