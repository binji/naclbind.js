#include "multi.h"
#include <stdlib.h>
#include <string.h>
#include "var.h"

void* my_malloc(size_t size) {
  return malloc(size);
}

void my_memcpy(void* dst, void* src, size_t len) {
  memcpy(dst, src, len);
}

void my_free(void* p) {
  free(p);
}

void rot13(char* s, size_t len) {
  int i;
  for (i = 0; i < len; ++i) {
    if (*s >= 'a' && *s <= 'z') {
      *s = (((*s - 'a') + 13) % 26) + 'a';
    } else if (*s >= 'A' && *s <= 'Z') {
      *s = (((*s - 'A') + 13) % 26) + 'A';
    }

    s++;
  }
}

struct PP_Var char_to_var(char* s) {
  return nb_var_string_create(s, strlen(s));
}

void var_release(struct PP_Var var) {
  nb_var_release(var);
}
