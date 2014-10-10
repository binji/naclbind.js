#include <assert.h>

int get_int(void) {
  return 1;
}

int do_stuff(void) {
  /* Shouldn't run */
  assert(0);
};
