typedef int (*func_t)(int);

int twice(int);
/* TODO(binji): This function shouldn't be necessary. Find a better way. */
func_t get_twice(void);
int do_42(func_t fp);
