#include "function_pointers.h"

int twice(int x) {
  return x * 2;
}

int do_42(func_t fp) {
  return fp(42);
}
