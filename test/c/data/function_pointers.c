#include "function_pointers.h"

int twice(int x) {
  return x * 2;
}

func_t get_twice(void) {
  return twice;
}

int do_42(func_t fp) {
  return fp(42);
}
