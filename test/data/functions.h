struct s1 {};
struct s2 { int f; };
union u1 {};

// Pointers.
void f1(void*);
void f2(struct s1*);
void f3(union u1*);

// Multiple params
void f4(int, int, void*);
void f5(int, struct s2);

// Return values
int f6(int);
void* f7(unsigned int);
struct s1 f8(void);

// Function pointers
int f9(void (*)(int));
void (*f10(int, void (*)(int)))(int);

// Arrays
void f11(int[]);
void f12(int[10]);
void f13(int argc, char* argv[]);
