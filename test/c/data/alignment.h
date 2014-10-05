struct S1 {
  void* voidp;
  signed char s8;
  unsigned char u8;
  signed short s16;
  unsigned short u16;
  signed int s32;
  unsigned int u32;
  signed long slong;
  unsigned long ulong;
  signed long long s64;
  unsigned long long u64;
  float f32;
  double f64;
};

struct S2 {
  double f64;
  float f32;
  unsigned long long u64;
  signed long long s64;
  unsigned long ulong;
  signed long slong;
  unsigned int u32;
  signed int s32;
  unsigned short u16;
  signed short s16;
  unsigned char u8;
  signed char s8;
  void* voidp;
};

void dummy1(struct S1* s);
void dummy2(struct S2* s);
