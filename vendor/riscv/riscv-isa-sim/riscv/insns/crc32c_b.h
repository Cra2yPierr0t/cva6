require_extension(EXT_XZBR);
reg_t x = zext_xlen(RS1);
for (int i = 0; i < 8; i++)
  x = (x >> 1) ^ (0x82F63B78 & ~((x&1)-1));
WRITE_RD(sext_xlen(x));
