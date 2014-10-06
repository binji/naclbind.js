#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

double sum(const char* f, ...) {
  fprintf(stderr, "sum: %s\n", f);
  double total = 0;
  va_list args;
  va_start(args, f);
  while (*f) {
    switch (*f) {
      case 'i': {
        int val = va_arg(args, int);
        fprintf(stderr, "got int: %d\n", val);
        total += val;
        break;
      }
      case 'l': {
        long long val = va_arg(args, long long);
        fprintf(stderr, "got long long: %lld\n", val);
        total += val;
        break;
      }
      case 'd': {
        double val = va_arg(args, double);
        fprintf(stderr, "got double: %g\n", val);
        total += val;
        break;
      }
      case 'p': {
        int* val = va_arg(args, int*);
        fprintf(stderr, "got int*: %p\n", val);
        total += *val;
        break;
      }
    }
    f++;
  }
  va_end(args);
  return total;
}

int* create_intp(int x) {
  int* p = malloc(sizeof(int));
  *p = x;
  return p;
}

void destroy_intp(int* p) {
  free(p);
}
