#include "enum.h"

enum e1 next(enum e1 x) {
  return x == E4 ? E1 : (enum e1)(x + 1);
}

enum e1 prev(enum e1 x) {
  return x == E1 ? E4 : (enum e1)(x - 1);
}
