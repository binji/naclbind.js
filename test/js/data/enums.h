enum e1 { E0, E1 };

typedef enum e2 {
  E2 = 2,
  E3,
  E4,
  E100 = 100,
} t1;

typedef enum {
  E5 = 5,
  E6,
} t2;

// Another anonymous enum
typedef enum {
  E7 = 7
} t3;

void f1(enum e1);
void f2(t1);
