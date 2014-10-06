typedef char t1;
typedef void *t2;
typedef t1 *t3;

typedef struct s1 t4;
struct s1 { t4* f; };
typedef t4 t5;

void f1(t1);
void f2(t2);
void f3(t3);
void f4(t5*);
