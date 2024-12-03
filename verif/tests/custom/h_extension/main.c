typedef unsigned long uint64_t;

#define M_MODE 0x11
#define HS_MODE 0x01
#define VS_MODE 0x01

#define V_MODE 0x1

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

void enter_hsmode() {
  uint64_t mstatus;
  asm volatile("csrr  %0      , mstatus" : "=r" (mstatus));
  mstatus = mstatus | (HS_MODE << 11);
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
  hstatus = hstatus | (V_MODE << 7);
  sstatus = sstatus | (VS_MODE << 8);
  asm volatile("csrw  hstatus , %0" : : "r" (hstatus));
  asm volatile("csrw  sstatus , %0" : : "r" (sstatus));
  asm volatile("la    t0      , _now_vsmode");
  asm volatile("csrw  sepc    , t0");
  asm volatile("sret");
  asm volatile("_now_vsmode:"); // here is in VS-mode
  return;
}

int main() {
  init_pmp();

  enter_hsmode();

  enter_vsmode();

  int a = 0x114514;

  return 0;
}
