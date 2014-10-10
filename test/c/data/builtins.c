#include "builtins.h"
#include <stdlib.h>

void* my_malloc(my_size_t size) {
  return malloc(size);
}

void my_free(void* p) {
  free(p);
}
