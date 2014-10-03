#include <ppapi/c/pp_var.h>

typedef unsigned int size_t;

void* my_malloc(size_t size);
void my_memcpy(void* dst, void* src, size_t len);
void my_free(void* p);
void rot13(char* s, size_t len);

// TODO(binji): Add support to directly convert char* -> String PP_Var
struct PP_Var char_to_var(char* s);
void var_release(struct PP_Var var);
