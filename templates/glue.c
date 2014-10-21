/* Copyright 2014 Ben Smith. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* DO NOT EDIT, this file is auto-generated from //templates/glue.c */

#define NB_ONE_FILE

{{IncludeFile('c/bool.h')}}
{{IncludeFile('c/error.h')}}
{{IncludeFile('c/handle.h')}}
{{IncludeFile('c/interfaces.h')}}
{{IncludeFile('c/macros.h')}}
{{IncludeFile('c/message.h')}}
{{IncludeFile('c/queue.h')}}
{{IncludeFile('c/run.h')}}
{{IncludeFile('c/type.h')}}
{{IncludeFile('c/var.h')}}
[[if builtins:]]
{{IncludeFile('c/builtins.h')}}
[[]]

{{IncludeFile('c/handle.c')}}
{{IncludeFile('c/interfaces.c')}}
{{IncludeFile('c/message.c')}}
{{IncludeFile('c/run.c')}}
{{IncludeFile('c/type.c')}}
{{IncludeFile('c/var.c')}}

#ifndef NB_NO_APP
{{IncludeFile('c/queue.c')}}
{{IncludeFile('c/app.c')}}
#endif

/* ========================================================================== */

#include "{{filename}}"
#include <stdarg.h>

#define NB_MAX_INT_VARARGS {{MAX_INT_VARARGS}}
#define NB_MAX_DBL_VARARGS {{MAX_DBL_VARARGS}}

[[[
def FuncCall(fname, nargs, iargs, dargs):
  args = []
  for i in xrange(nargs):
    args.append('arg%d' % i)
  for i in xrange(iargs):
    args.append('iargs[%d]' % i)
  for i in xrange(dargs):
    args.append('dargs[%d]' % i)
  return '%s(%s)' % (fname, ', '.join(args))
]]]

[[for type in collector.types_topo:]]
[[  if type.kind != TypeKind.RECORD or type.is_anonymous:]]
[[    continue]]
[[  ]]
[[  if type.size > 0:]]
NB_COMPILE_ASSERT(sizeof({{type.c_spelling}}) == {{type.size}});
[[  for name, ftype, offset in type.fields:]]
NB_COMPILE_ASSERT(offsetof({{type.c_spelling}}, {{name}}) == {{offset}});
[[  ]]
[[]]

[[for fn in collector.functions:]]
// {{fn.displayname}}
static NB_Bool nb_command_run_{{fn.spelling}}(struct NB_Message* message, int command_idx) {
[[  if fn.type.kind == TypeKind.FUNCTIONPROTO:]]
[[    arguments = list(fn.type.arg_types)]]
  int arg_count = nb_message_command_arg_count(message, command_idx);
[[    if fn.type.is_variadic:]]
  if (arg_count < {{len(arguments)}}) {
    NB_VERROR("Expected at least %d args, got %d.", {{len(arguments)}}, arg_count);
[[    else:]]
  if (arg_count != {{len(arguments)}}) {
    NB_VERROR("Expected %d args, got %d.", {{len(arguments)}}, arg_count);
[[    ]]
    return NB_FALSE;
  }
[[    for i, arg in enumerate(arguments):]]
[[      orig_arg, arg = arg, arg.canonical]]
  NB_Handle handle{{i}} = nb_message_command_arg(message, command_idx, {{i}});
[[      if arg.kind == TypeKind.POINTER:]]
[[        pointee = arg.pointee]]
[[        if pointee.kind in (TypeKind.CHAR_S, TypeKind.CHAR_U):]]
  char* arg{{i}};
  if (!nb_handle_get_charp(handle{{i}}, &arg{{i}})) {
    NB_VERROR("Unable to get handle %d as char*.", handle{{i}});
    return NB_FALSE;
  }
[[        elif pointee.kind == TypeKind.VOID:]]
  void* arg{{i}};
  if (!nb_handle_get_voidp(handle{{i}}, &arg{{i}})) {
    NB_VERROR("Unable to get handle %d as void*.", handle{{i}});
    return NB_FALSE;
  }
[[        elif pointee.kind == TypeKind.FUNCTIONPROTO:]]
  void (*arg{{i}}x)(void);
  if (!nb_handle_get_funcp(handle{{i}}, &arg{{i}}x)) {
    NB_VERROR("Unable to get handle %d as void(*)(void).", handle{{i}});
    return NB_FALSE;
  }
  {{arg.GetCSpelling('arg%s' % i)}} = ({{arg.c_spelling}}) arg{{i}}x;
[[        else:]]
  void* arg{{i}}x;
  if (!nb_handle_get_voidp(handle{{i}}, &arg{{i}}x)) {
    NB_VERROR("Unable to get handle %d as void*.", handle{{i}});
    return NB_FALSE;
  }
  {{arg.c_spelling}} arg{{i}} = ({{arg.c_spelling}}) arg{{i}}x;
[[      elif arg.kind == TypeKind.LONG:]]
  int32_t arg{{i}}x;
  if (!nb_handle_get_int32(handle{{i}}, &arg{{i}}x)) {
    NB_VERROR("Unable to get handle %d as int32_t.", handle{{i}});
    return NB_FALSE;
  }
  long arg{{i}} = (long) arg{{i}}x;
[[      elif arg.kind == TypeKind.ULONG:]]
  uint32_t arg{{i}}x;
  if (!nb_handle_get_uint32(handle{{i}}, &arg{{i}}x)) {
    NB_VERROR("Unable to get handle %d as uint32_t.", handle{{i}});
    return NB_FALSE;
  }
  unsigned long arg{{i}} = (unsigned long) arg{{i}}x;
[[      elif arg.kind == TypeKind.LONGLONG:]]
  int64_t arg{{i}};
  if (!nb_handle_get_int64(handle{{i}}, &arg{{i}})) {
    NB_VERROR("Unable to get handle %d as int64_t.", handle{{i}});
    return NB_FALSE;
  }
[[      elif arg.kind == TypeKind.ULONGLONG:]]
  uint64_t arg{{i}};
  if (!nb_handle_get_uint64(handle{{i}}, &arg{{i}})) {
    NB_VERROR("Unable to get handle %d as uint64_t.", handle{{i}});
    return NB_FALSE;
  }
[[      elif arg.kind in (TypeKind.INT, TypeKind.SHORT, TypeKind.SCHAR, TypeKind.CHAR_S):]]
  int32_t arg{{i}};
  if (!nb_handle_get_int32(handle{{i}}, &arg{{i}})) {
    NB_VERROR("Unable to get handle %d as int32_t.", handle{{i}});
    return NB_FALSE;
  }
[[      elif arg.kind in (TypeKind.UINT, TypeKind.USHORT, TypeKind.UCHAR, TypeKind.CHAR_U):]]
  uint32_t arg{{i}};
  if (!nb_handle_get_uint32(handle{{i}}, &arg{{i}})) {
    NB_VERROR("Unable to get handle %d as uint32_t.", handle{{i}});
    return NB_FALSE;
  }
[[      elif arg.kind == TypeKind.FLOAT:]]
  float arg{{i}};
  if (!nb_handle_get_float(handle{{i}}, &arg{{i}})) {
    NB_VERROR("Unable to get handle %d as float.", handle{{i}});
    return NB_FALSE;
  }
[[      elif arg.kind == TypeKind.DOUBLE:]]
  double arg{{i}};
  if (!nb_handle_get_double(handle{{i}}, &arg{{i}})) {
    NB_VERROR("Unable to get handle %d as double.", handle{{i}});
    return NB_FALSE;
  }
[[      elif arg.kind == TypeKind.ENUM:]]
  int32_t arg{{i}}x;
  if (!nb_handle_get_int32(handle{{i}}, &arg{{i}}x)) {
    NB_VERROR("Unable to get handle %d as int32_t.", handle{{i}});
    return NB_FALSE;
  }
  {{arg.c_spelling}} arg{{i}} = ({{arg.c_spelling}}) arg{{i}}x;
[[      elif arg.kind == TypeKind.RECORD and arg.c_spelling == 'struct PP_Var':]]
  struct PP_Var arg{{i}};
  if (!nb_handle_get_var(handle{{i}}, &arg{{i}})) {
    NB_VERROR("Unable to get handle %d as struct PP_Var.", handle{{i}});
    return NB_FALSE;
  }
[[      elif arg.kind == TypeKind.CONSTANTARRAY:]]
  // UNSUPPORTED: {{arg.kind}} {{arg.c_spelling}} {{orig_arg.c_spelling}}
[[        if orig_arg.c_spelling == '__gnuc_va_list':]]
  (void)handle{{i}};
  va_list arg{{i}};
  NB_ERROR("va_lists are not currently supported.");
[[        else:]]
  (void)handle{{i}};
  {{arg.element_type.c_spelling}}* arg{{i}} = NULL;
  NB_ERROR("Arrays are not currently supported.");
[[      elif arg.kind == TypeKind.INCOMPLETEARRAY:]]
  // UNSUPPORTED: {{arg.kind}} {{arg.c_spelling}} {{orig_arg.c_spelling}}
  (void)handle{{i}};
  {{arg.element_type.c_spelling}}* arg{{i}} = NULL;
  NB_ERROR("Arrays are not currently supported.");
[[      elif arg.kind == TypeKind.RECORD:]]
  (void)handle{{i}};
  {{arg.c_spelling}} arg{{i}};
  NB_ERROR("Passing structs and unions by value is not currently supported.");
[[      else:]]
  // UNSUPPORTED: {{arg.kind}} {{arg.c_spelling}}
  NB_ERROR("Type {{arg.c_spelling}} is not currently supported.");
[[    ]]
[[    if fn.type.is_variadic:]]
  NB_VarArgInt iargs[NB_MAX_INT_VARARGS];
  NB_VarArgInt* iargsp = iargs;
  NB_VarArgInt* iargs_end = &iargs[NB_MAX_INT_VARARGS];
  NB_VarArgDbl dargs[NB_MAX_DBL_VARARGS];
  NB_VarArgDbl* dargsp = dargs;
  NB_VarArgDbl* dargs_end = &dargs[NB_MAX_DBL_VARARGS];
  int i;
  for (i = 0; i < arg_count - {{len(arguments)}}; ++i) {
    NB_Handle handle = nb_message_command_arg(message, command_idx, i + {{len(arguments)}});
    if (!nb_handle_get_default(handle, &iargsp, iargs_end,
                                       &dargsp, dargs_end)) {
      NB_ERROR("Failed to add variadic argument.");
      return NB_FALSE;
    }
  }
  size_t iarg_count = iargsp - iargs;
  size_t darg_count = dargsp - dargs;
[[  elif fn.type.kind == TypeKind.FUNCTIONNOPROTO:]]
  int arg_count = nb_message_command_arg_count(message, command_idx);
  if (arg_count != 0) {
    // UNSUPPORTED: no proto with non-zero argument list.
    NB_VERROR("Function with no prototype passed non-zero args: %d", arg_count);
    return NB_FALSE;
  }
[[    arguments = []]]
[[  else:]]
[[    raise Error('Unexpected function type: %s' % fn.type.kind)]]
[[  result_type = fn.type.result_type.canonical]]
[[  if result_type.kind != TypeKind.VOID:]]
  if (!nb_message_command_has_ret(message, command_idx)) {
    NB_ERROR("Return type is non-void, but no return handle given.");
    return NB_FALSE;
  }
  NB_Handle ret = nb_message_command_ret(message, command_idx);
[[    if fn.type.kind == TypeKind.FUNCTIONPROTO and fn.type.is_variadic:]]
  {{result_type.GetCSpelling('result')}};
#ifdef __x86_64__
  /* This relies on the fact that the x86_64 calling convention for variadic
   * functions does not preserve ordering w.r.t. floating-point values. We can
   * push them all to the end of the call and they still will be retrieved in
   * the correct order by the callee.
   */
  switch (iarg_count) {
[[      for j in range(MAX_INT_VARARGS + 1):]]
    case {{j}}:
      switch (darg_count) {
[[        for k in range(MAX_DBL_VARARGS + 1):]]
        case {{k}}: result = {{FuncCall(fn.spelling, len(arguments), j, k)}}; break;
[[        ]]
        default: assert(!"darg_count >= {{MAX_DBL_VARARGS + 1}}"); return NB_FALSE;
      }
      break;
[[      ]]
    default: assert(!"iarg_count >= {{MAX_INT_VARARGS + 1}}"); return NB_FALSE;
  }
#else
  (void)darg_count;
  switch (iarg_count) {
[[      for j in range(MAX_INT_VARARGS + 1):]]
    case {{j}}: result = {{FuncCall(fn.spelling, len(arguments), j, 0)}}; break;
[[      ]]
    default: assert(!"iarg_count >= {{MAX_INT_VARARGS + 1}}"); return NB_FALSE;
  }
#endif
[[    else:]]
  {{result_type.GetCSpelling('result')}} = {{FuncCall(fn.spelling, len(arguments), 0, 0)}};
[[    ]]
[[    if result_type.kind in (TypeKind.SCHAR, TypeKind.CHAR_S):]]
  NB_Bool register_ok = nb_handle_register_int8(ret, result);
[[    elif result_type.kind in (TypeKind.UCHAR, TypeKind.CHAR_U):]]
  NB_Bool register_ok = nb_handle_register_uint8(ret, result);
[[    elif result_type.kind == TypeKind.SHORT:]]
  NB_Bool register_ok = nb_handle_register_int16(ret, result);
[[    elif result_type.kind == TypeKind.USHORT:]]
  NB_Bool register_ok = nb_handle_register_uint16(ret, result);
[[    elif result_type.kind == TypeKind.INT:]]
  NB_Bool register_ok = nb_handle_register_int32(ret, result);
[[    elif result_type.kind == TypeKind.UINT:]]
  NB_Bool register_ok = nb_handle_register_uint32(ret, result);
[[    elif result_type.kind == TypeKind.LONG:]]
  NB_Bool register_ok = nb_handle_register_int32(ret, (int32_t) result);
[[    elif result_type.kind == TypeKind.ULONG:]]
  NB_Bool register_ok = nb_handle_register_uint32(ret, (uint32_t) result);
[[    elif result_type.kind == TypeKind.LONGLONG:]]
  NB_Bool register_ok = nb_handle_register_int64(ret, result);
[[    elif result_type.kind == TypeKind.ULONGLONG:]]
  NB_Bool register_ok = nb_handle_register_uint64(ret, result);
[[    elif result_type.kind == TypeKind.FLOAT:]]
  NB_Bool register_ok = nb_handle_register_float(ret, result);
[[    elif result_type.kind == TypeKind.DOUBLE:]]
  NB_Bool register_ok = nb_handle_register_double(ret, result);
[[    elif result_type.kind == TypeKind.POINTER:]]
[[      pointee = result_type.pointee]]
[[      if pointee.kind == TypeKind.FUNCTIONPROTO:]]
  NB_Bool register_ok = nb_handle_register_funcp(ret, (void(*)(void))result);
[[      else:]]
  NB_Bool register_ok = nb_handle_register_voidp(ret, (void*) result);
[[      ]]
[[    elif result_type.kind == TypeKind.RECORD and result_type.c_spelling == 'struct PP_Var':]]
  NB_Bool register_ok = nb_handle_register_var(ret, result);
[[    elif result_type.kind == TypeKind.ENUM:]]
  NB_Bool register_ok = nb_handle_register_int32(ret, (int32_t) result);
[[    else:]]
  // UNSUPPORTED: {{result_type.kind}} {{result_type.c_spelling}}
  (void)result;
  NB_Bool register_ok = NB_FALSE;
[[    ]]
  if (!register_ok) {
    NB_VERROR("Failed to register handle %d of type {{result_type.c_spelling}}.", ret);
    return NB_FALSE;
  }
  return NB_TRUE;
[[  else:]]
  {{fn.spelling}}({{', '.join('arg%d' % i for i in range(len(arguments)))}});
  return NB_TRUE;
[[  ]]
}

[[]]

// getFunc()
static NB_Bool nb_command_run_get_func(struct NB_Message* message, int command_idx) {
  int arg_count = nb_message_command_arg_count(message, command_idx);
  if (arg_count != 1) {
    NB_VERROR("Expected %d arg, got %d.", 1, arg_count);
    return NB_FALSE;
  }
  NB_Handle handle = nb_message_command_arg(message, command_idx, 0);
  int32_t arg;
  if (!nb_handle_get_int32(handle, &arg)) {
    NB_VERROR("Unable to get handle %d as int32_t.", handle);
    return NB_FALSE;
  }
  if (!nb_message_command_has_ret(message, command_idx)) {
    NB_ERROR("Return type is non-void, but no return handle given.");
    return NB_FALSE;
  }
  NB_Handle ret = nb_message_command_ret(message, command_idx);

  void (*result)(void);
  switch (arg) {
[[for fn in collector.functions:]]
    case {{fn.fn_id}}: result = (void(*)(void)) &{{fn.spelling}}; break;
[[]]
    default: return NB_FALSE;
  }

  NB_Bool register_ok = nb_handle_register_funcp(ret, result);
  if (!register_ok) {
    NB_VERROR("Failed to register handle %d of type void(*)(void).", ret);
    return NB_FALSE;
  }
  return NB_TRUE;
}

// $errorIf()
static NB_Bool nb_command_run_error_if(struct NB_Message* message, int command_idx) {
  int arg_count = nb_message_command_arg_count(message, command_idx);
  if (arg_count != 1) {
    NB_VERROR("Expected %d arg, got %d.", 1, arg_count);
    return NB_FALSE;
  }
  NB_Handle handle = nb_message_command_arg(message, command_idx, 0);
  int32_t arg;
  if (!nb_handle_get_int32(handle, &arg)) {
    NB_VERROR("Unable to get handle %d as int32_t.", handle);
    return NB_FALSE;
  }
  return arg != 0 ? NB_FALSE : NB_TRUE;
}

enum {
  NUM_FUNCTIONS = {{len(collector.functions)}}
};

typedef NB_Bool (*nb_command_func_t)(struct NB_Message*, int);
static nb_command_func_t s_functions[] = {
  nb_command_run_get_func,  /* -2 */
  nb_command_run_error_if,  /* -1 */
[[for fn in collector.functions:]]
  nb_command_run_{{fn.spelling}},  /* {{fn.fn_id}} */
[[]]
};

NB_Bool nb_message_command_run(struct NB_Message* message, int command_idx) {
  int function_idx = nb_message_command_function(message, command_idx);
  if (function_idx < -2 || function_idx >= NUM_FUNCTIONS) {
    NB_VERROR("Function id %d is out of range [-2, %d).", function_idx, NUM_FUNCTIONS);
    return NB_FALSE;
  }

  return s_functions[function_idx + 2](message, command_idx);
}
