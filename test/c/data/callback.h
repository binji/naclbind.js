typedef int (*int_func)(int);
int call_with_10_and_add_1(int_func f);

typedef long long int int64;
typedef int64 (*int64_func)(int64);
int64 sum_calls_of_10_and_20(int64_func f);
