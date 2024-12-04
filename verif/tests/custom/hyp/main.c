typedef unsigned long uint64_t;

#define M_MODE 0x11
#define HS_MODE 0x01
#define VS_MODE 0x01
#define VU_MODE 0x00

#define V_MODE 0x1

#define MSTATUS_MPP 11
#define HSTATUS_SPV 7
#define SSTATUS_SPP 8

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

int main() {
  init_pmp();

  enter_hsmode();

  enter_vsmode();

  return 0;
}
