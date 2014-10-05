#include <ppapi/c/pp_var.h>
#include <stdlib.h>
#include <string.h>

// naclbind-gen: -W malloc -W free -W memcpy
void rot13(char* s, size_t len);

// TODO(binji): Add support to directly convert char* -> String PP_Var
struct PP_Var char_to_var(char* s);
void var_release(struct PP_Var var);
