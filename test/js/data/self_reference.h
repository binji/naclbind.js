struct A {
  struct B* b;
};

struct B {
  struct A* a;
};

void f(struct A*);
