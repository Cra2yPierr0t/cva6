typedef unsigned long uint64_t;

#define M_MODE 0x11
#define S_MODE 0x01
#define U_MODE 0x00

#define PTE_V 0x1
#define PTE_R 0x2
#define PTE_W 0x4
#define PTE_X 0x8
#define PTE_A 0x40
#define PTE_D 0x80

#define MMU_EN 0x8000000000000000

#define TEXT_ADDR 0x80000000
#define L3_PAGETABLE 0x80010000 // under .text
#define L2_PAGETABLE 0x80020000
#define L1_PAGETABLE 0x80030000
#define L2_PAGETABLE_D 0x80080000
#define L1_PAGETABLE_D 0x80090000

void init_pmp() {
  //S-mode can access 0x00000000 ~ 0xffffffff range
  asm volatile("li    t0      , 0x0000001f");
  asm volatile("csrw  pmpcfg0 , t0");
  asm volatile("li    t0      , 0xffffffff");
  asm volatile("csrw  pmpaddr0, t0");
  return;
}

void enter_smode() {
  uint64_t mstatus;
  asm volatile("csrr  %0      , mstatus" : "=r" (mstatus));
  mstatus = mstatus | (S_MODE << 11);
  asm volatile("csrw  mstatus , %0" : : "r" (mstatus));
  asm volatile("la    t0      , _now_smode");
  asm volatile("csrw  mepc    , t0");
  asm volatile("mret");
  asm volatile("_now_smode:"); // here is in S-mode
  return;
}

void init_mmu() {
  uint64_t satp = 0;
  uint64_t *pte;
  // L3 Page Table Head is 0x80010000
  satp = (L3_PAGETABLE >> 12) & 0x00000fffffffffff;

  // direct mapping for instruction fetch
  // 0x80000000 -> 0x80000000
  // 0x80001000 -> 0x80000000
  // ...
  // 0x80040000 -> 0x80040000 0x41 Pages 260 KiB

  // VPN[2],0b000 = (0x80000000 >> 30) << 3
  pte = L3_PAGETABLE | (0x80000000 >> 30) << 3;
  *pte = ((L2_PAGETABLE >> 12) << 10) | PTE_V;

  // VPN[1],0b000 = ((0x80000000 & 0x3fe00000) >> 21) << 3
  pte = L2_PAGETABLE | ((0x80000000 & 0x3fe00000) >> 21) << 3;
  *pte = ((L1_PAGETABLE >> 12) << 10) | PTE_V;

  // VPN[0],0b000 = ((0x80000000 & 0x1ff000) >> 12) << 3
  for(int i = 0; i < 0x41; i++) {
    pte = L1_PAGETABLE | (((0x80000000 + (i * 0x1000)) & 0x1ff000) >> 12) << 3;
    *pte = (((0x80000000 + (i * 0x1000)) >> 12) << 10) | PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
  }

  // mapping for data access
  // VA : 0x00000000 -> PA: 0x80003000
  // VA : 0x00001000 -> PA: 0x80004000
  // VA : 0x00002000 -> PA: 0x80005000
  // VA : 0x00003000 -> PA: 0x80006000 0x4 Pages 16 KiB

  // VPN[2],0b000 = (0x00000000 >> 30) << 3
  pte = L3_PAGETABLE | (0x00000000 >> 30) << 3;
  *pte = ((L2_PAGETABLE_D >> 12) << 10) | PTE_V;

  // VPN[1],0b000 = ((0x00000000 & 0x3fe00000) >> 21) << 3
  pte = L2_PAGETABLE_D | ((0x00000000 & 0x3fe00000) >> 21) << 3;
  *pte = ((L1_PAGETABLE_D >> 12) << 10) | PTE_V;

  // VPN[0],0b000 = ((0x00000000 & 0x1ff000) >> 12) << 3
  for(int i = 0; i < 0x4; i++) {
    pte = L1_PAGETABLE_D | (((0x00000000 + (i * 0x1000)) & 0x1ff000) >> 12) << 3;
    *pte = (((0x80003000 + (i * 0x1000)) >> 12) << 10) | PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
  }

  // activate MMU
  satp |= MMU_EN;
  asm volatile("csrw satp, %0" : : "r" (satp));
  asm volatile("sfence.vma zero, zero");
}

int main() {
  init_pmp();

  enter_smode();

  init_mmu();

  int a = 0x114514;
  uint64_t *mem;
  mem = (uint64_t *)0x00000040;
  *mem = a;

  return 0;
}
