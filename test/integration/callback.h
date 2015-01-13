int call_with_10_and_add_1(int(*)(int));
int call_n_down_to_1_and_sum(int n, int(*)(int));

int* init_data(int n, int(*value)(int index));
void free_data(int*);
void iter_data(int n, int* data, int(*each)(int index, int value));
void selection_sort(int n, int* data, int (*cmp)(int, int));
