#include "callback.h"

int call_with_10_and_add_1(int_func f) {
  return f(10) + 1;
}

int64 sum_calls_of_10_and_20(int64_func f) {
  return f(10) + f(20);
}
