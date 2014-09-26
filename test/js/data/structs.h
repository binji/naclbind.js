// Incomplete struct
struct s1;

// Empty struct
struct s2 {};

// Basic struct
struct s3 {
  int f;
};

// Struct with typedef
typedef struct s4 {
  int g;
} t1;

// Anonymous nested struct, named field
struct s5 {
  struct {
    int f;
    int g;
  } nested;
};

// Anonymous nested struct, unnamed field
struct s6 {
  struct {
    int f;
    int g;
  };
};

// Named nested struct, named field
struct s7 {
  struct ns1 {
    int f;
  } g;
  int h;
};

// Named nested struct, unnamed field
struct s8 {
  struct ns2 {
    int f;
  };
  int g;
};

void f1(struct s1);
void f2(struct s2);
void f3(struct s3);
void f4(t1);
void f5(struct s5);
void f6(struct s6);
void f7(struct s7);
void f8(struct s8);
