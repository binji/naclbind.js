#include <stdlib.h>

int call_with_10_and_add_1(int(*func)(int)) {
  return func(10) + 1;
}

int call_n_down_to_1_and_sum(int n, int(*func)(int)) {
  int sum = 0;
  while (n > 0) {
    sum += func(n);
    n--;
  }
  return sum;
}

int* init_data(int n, int(*value)(int index)) {
  int i;
  int* data = malloc(n * sizeof(int));
  for (i = 0; i < n; i++) {
    data[i] = value(i);
  }
  return data;
}

void free_data(int* data) {
  free(data);
}

void iter_data(int n, int* data, int(*each)(int index, int value)) {
  int i;
  for (i = 0; i < n; i++) {
    each(i, data[i]);
  }
}

void selection_sort(int n, int* data, int (*cmp)(int, int)) {
  int i, j;
  for (i = 0; i < n - 1; i++) {
    int min_value = data[i];
    int min_index = i;
    for (j = i + 1; j < n; j++) {
      if (cmp(data[j], min_value) < 0) {
        min_value = data[j];
        min_index = j;
      }
    }

    if (min_index != i) {
      data[min_index] = data[i];
      data[i] = min_value;
    }
  }
}
