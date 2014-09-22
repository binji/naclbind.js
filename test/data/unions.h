// Incomplete union
union s1;

// Empty union
union s2 {};

// Basic union
union s3 {
  int f;
};

// Union with typedef
typedef union s4 {
  int g;
} t1;

// Anonymous nested union, named field
union s5 {
  union {
    int f;
    int g;
  } nested;
};

// Anonymous nested union, unnamed field
union s6 {
  union {
    int f;
    int g;
  };
};

// Named nested union, named field
union s7 {
  union ns1 {
    int f;
  } g;
  int h;
};

// Named nested union, unnamed field
union s8 {
  union ns2 {
    int f;
  };
  int g;
};

void f1(union s1);
void f2(union s2);
void f3(union s3);
void f4(t1);
void f5(union s5);
void f6(union s6);
void f7(union s7);
void f8(union s8);
