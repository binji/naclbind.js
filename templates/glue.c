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

#define MAX_INT_VARARGS {{MAX_INT_VARARGS}}
#define MAX_DBL_VARARGS {{MAX_DBL_VARARGS}}

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
[[  if type.kind != TypeKind.RECORD or type.IsAnonymous():]]
[[    continue]]
[[  ]]
[[  if type.get_size() > 0:]]
COMPILE_ASSERT(sizeof({{type.spelling}}) == {{type.get_size()}});
[[  for name, ftype, offset in type.fields():]]
COMPILE_ASSERT(offsetof({{type.spelling}}, {{name}}) == {{offset}});
[[  ]]
[[]]

[[for fn in collector.functions:]]
// {{fn.displayname}}
static bool nb_command_run_{{fn.spelling}}(struct Message* message, int command_idx) {
[[  if fn.type.kind == TypeKind.FUNCTIONPROTO:]]
[[    arguments = list(fn.type.argument_types())]]
  int arg_count = nb_message_command_arg_count(message, command_idx);
[[    if fn.type.is_function_variadic():]]
  if (arg_count < {{len(arguments)}}) {
    VERROR("Expected at least %d args, got %d.", {{len(arguments)}}, arg_count);
[[    else:]]
  if (arg_count != {{len(arguments)}}) {
    VERROR("Expected %d args, got %d.", {{len(arguments)}}, arg_count);
[[    ]]
    return FALSE;
  }
[[    for i, arg in enumerate(arguments):]]
[[      orig_arg, arg = arg, arg.get_canonical()]]
  Handle handle{{i}} = nb_message_command_arg(message, command_idx, {{i}});
[[      if arg.kind == TypeKind.POINTER:]]
[[        pointee = arg.get_pointee()]]
[[        if pointee.kind in (TypeKind.CHAR_S, TypeKind.CHAR_U):]]
  char* arg{{i}};
  if (!nb_handle_get_charp(handle{{i}}, &arg{{i}})) {
    VERROR("Unable to get handle %d as char*.", handle{{i}});
    return FALSE;
  }
[[        elif pointee.kind == TypeKind.VOID:]]
  void* arg{{i}};
  if (!nb_handle_get_voidp(handle{{i}}, &arg{{i}})) {
    VERROR("Unable to get handle %d as void*.", handle{{i}});
    return FALSE;
  }
[[        elif pointee.kind == TypeKind.FUNCTIONPROTO:]]
  // UNSUPPORTED: {{arg.kind}} {{arg.spelling}}
  (void)handle{{i}};
  void* arg{{i}} = NULL;
  ERROR("Function pointers are not currently supported.");
[[        else:]]
  void* arg{{i}}x;
  if (!nb_handle_get_voidp(handle{{i}}, &arg{{i}}x)) {
    VERROR("Unable to get handle %d as void*.", handle{{i}});
    return FALSE;
  }
  {{arg.spelling}} arg{{i}} = ({{arg.spelling}}) arg{{i}}x;
[[      elif arg.kind == TypeKind.LONG:]]
  int32_t arg{{i}}x;
  if (!nb_handle_get_int32(handle{{i}}, &arg{{i}}x)) {
    VERROR("Unable to get handle %d as int32_t.", handle{{i}});
    return FALSE;
  }
  long arg{{i}} = (long) arg{{i}}x;
[[      elif arg.kind == TypeKind.ULONG:]]
  uint32_t arg{{i}}x;
  if (!nb_handle_get_uint32(handle{{i}}, &arg{{i}}x)) {
    VERROR("Unable to get handle %d as uint32_t.", handle{{i}});
    return FALSE;
  }
  unsigned long arg{{i}} = (unsigned long) arg{{i}}x;
[[      elif arg.kind == TypeKind.LONGLONG:]]
  int64_t arg{{i}};
  if (!nb_handle_get_int64(handle{{i}}, &arg{{i}})) {
    VERROR("Unable to get handle %d as int64_t.", handle{{i}});
    return FALSE;
  }
[[      elif arg.kind == TypeKind.ULONGLONG:]]
  uint64_t arg{{i}};
  if (!nb_handle_get_uint64(handle{{i}}, &arg{{i}})) {
    VERROR("Unable to get handle %d as uint64_t.", handle{{i}});
    return FALSE;
  }
[[      elif arg.kind in (TypeKind.INT, TypeKind.SHORT, TypeKind.SCHAR, TypeKind.CHAR_S):]]
  int32_t arg{{i}};
  if (!nb_handle_get_int32(handle{{i}}, &arg{{i}})) {
    VERROR("Unable to get handle %d as int32_t.", handle{{i}});
    return FALSE;
  }
[[      elif arg.kind in (TypeKind.UINT, TypeKind.USHORT, TypeKind.UCHAR, TypeKind.CHAR_U):]]
  uint32_t arg{{i}};
  if (!nb_handle_get_uint32(handle{{i}}, &arg{{i}})) {
    VERROR("Unable to get handle %d as uint32_t.", handle{{i}});
    return FALSE;
  }
[[      elif arg.kind == TypeKind.FLOAT:]]
  float arg{{i}};
  if (!nb_handle_get_float(handle{{i}}, &arg{{i}})) {
    VERROR("Unable to get handle %d as float.", handle{{i}});
    return FALSE;
  }
[[      elif arg.kind == TypeKind.DOUBLE:]]
  double arg{{i}};
  if (!nb_handle_get_double(handle{{i}}, &arg{{i}})) {
    VERROR("Unable to get handle %d as double.", handle{{i}});
    return FALSE;
  }
[[      elif arg.kind == TypeKind.ENUM:]]
  int32_t arg{{i}}x;
  if (!nb_handle_get_int32(handle{{i}}, &arg{{i}}x)) {
    VERROR("Unable to get handle %d as int32_t.", handle{{i}});
    return FALSE;
  }
  {{arg.spelling}} arg{{i}} = ({{arg.spelling}}) arg{{i}}x;
[[      elif arg.kind == TypeKind.RECORD and arg.spelling == 'struct PP_Var':]]
  struct PP_Var arg{{i}};
  if (!nb_handle_get_var(handle{{i}}, &arg{{i}})) {
    VERROR("Unable to get handle %d as struct PP_Var.", handle{{i}});
    return FALSE;
  }
[[      elif arg.kind == TypeKind.CONSTANTARRAY:]]
  // UNSUPPORTED: {{arg.kind}} {{arg.spelling}} {{orig_arg.spelling}}
[[        if orig_arg.spelling == '__gnuc_va_list':]]
  (void)handle{{i}};
  va_list arg{{i}};
  ERROR("va_lists are not currently supported.");
[[        else:]]
  (void)handle{{i}};
  {{arg.get_array_element_type().spelling}}* arg{{i}} = NULL;
  ERROR("Constant arrays are not currently supported.");
[[      elif arg.kind == TypeKind.RECORD:]]
  (void)handle{{i}};
  {{arg.spelling}} arg{{i}};
  ERROR("Passing structs and unions by value is not currently supported.");
[[      else:]]
  // UNSUPPORTED: {{arg.kind}} {{arg.spelling}}
  ERROR("Type {{arg.spelling}} is not currently supported.");
[[    ]]
[[    if fn.type.is_function_variadic():]]
  nb_vararg_int_t iargs[MAX_INT_VARARGS];
  nb_vararg_int_t* iargsp = iargs;
  nb_vararg_int_t* iargs_end = &iargs[MAX_INT_VARARGS];
  nb_vararg_dbl_t dargs[MAX_DBL_VARARGS];
  nb_vararg_dbl_t* dargsp = dargs;
  nb_vararg_dbl_t* dargs_end = &dargs[MAX_DBL_VARARGS];
  int i;
  for (i = 0; i < arg_count - {{len(arguments)}}; ++i) {
    Handle handle = nb_message_command_arg(message, command_idx, i + {{len(arguments)}});
    if (!nb_handle_get_default(handle, &iargsp, iargs_end,
                                       &dargsp, dargs_end)) {
      ERROR("Failed to add variadic argument.");
      return FALSE;
    }
  }
  size_t iarg_count = iargsp - iargs;
  size_t darg_count = dargsp - dargs;
[[  elif fn.type.kind == TypeKind.FUNCTIONNOPROTO:]]
  int arg_count = nb_message_command_arg_count(message, command_idx);
  if (arg_count != 0) {
    // UNSUPPORTED: no proto with non-zero argument list.
    VERROR("Function with no prototype passed non-zero args: %d", arg_count);
    return FALSE;
  }
[[    arguments = []]]
[[  else:]]
[[    raise Error('Unexpected function type: %s' % fn.type.kind)]]
[[  result_type = fn.type.get_result().get_canonical()]]
[[  if result_type.kind != TypeKind.VOID:]]
  if (!nb_message_command_has_ret(message, command_idx)) {
    ERROR("Return type is non-void, but no return handle given.");
    return FALSE;
  }
  Handle ret = nb_message_command_ret(message, command_idx);
[[    if fn.type.kind == TypeKind.FUNCTIONPROTO and fn.type.is_function_variadic():]]
  {{result_type.spelling}} result;
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
        default: assert(!"darg_count >= {{MAX_DBL_VARARGS + 1}}"); return FALSE;
      }
      break;
[[      ]]
    default: assert(!"iarg_count >= {{MAX_INT_VARARGS + 1}}"); return FALSE;
  }
#else
  (void)darg_count;
  switch (iarg_count) {
[[      for j in range(MAX_INT_VARARGS + 1):]]
    case {{j}}: result = {{FuncCall(fn.spelling, len(arguments), j, 0)}}; break;
[[      ]]
    default: assert(!"iarg_count >= {{MAX_INT_VARARGS + 1}}"); return FALSE;
  }
#endif
[[    else:]]
  {{result_type.spelling}} result = {{FuncCall(fn.spelling, len(arguments), 0, 0)}};
[[    ]]
[[    if result_type.kind in (TypeKind.SCHAR, TypeKind.CHAR_S):]]
  bool register_ok = nb_handle_register_int8(ret, result);
[[    elif result_type.kind in (TypeKind.UCHAR, TypeKind.CHAR_U):]]
  bool register_ok = nb_handle_register_uint8(ret, result);
[[    elif result_type.kind == TypeKind.SHORT:]]
  bool register_ok = nb_handle_register_int16(ret, result);
[[    elif result_type.kind == TypeKind.USHORT:]]
  bool register_ok = nb_handle_register_uint16(ret, result);
[[    elif result_type.kind == TypeKind.INT:]]
  bool register_ok = nb_handle_register_int32(ret, result);
[[    elif result_type.kind == TypeKind.UINT:]]
  bool register_ok = nb_handle_register_uint32(ret, result);
[[    elif result_type.kind == TypeKind.LONG:]]
  bool register_ok = nb_handle_register_int32(ret, (int32_t)result);
[[    elif result_type.kind == TypeKind.ULONG:]]
  bool register_ok = nb_handle_register_uint32(ret, (uint32_t)result);
[[    elif result_type.kind == TypeKind.LONGLONG:]]
  bool register_ok = nb_handle_register_int64(ret, result);
[[    elif result_type.kind == TypeKind.ULONGLONG:]]
  bool register_ok = nb_handle_register_uint64(ret, result);
[[    elif result_type.kind == TypeKind.FLOAT:]]
  bool register_ok = nb_handle_register_float(ret, result);
[[    elif result_type.kind == TypeKind.DOUBLE:]]
  bool register_ok = nb_handle_register_double(ret, result);
[[    elif result_type.kind == TypeKind.POINTER:]]
  bool register_ok = nb_handle_register_voidp(ret, result);
[[    elif result_type.kind == TypeKind.RECORD and result_type.spelling == 'struct PP_Var':]]
  bool register_ok = nb_handle_register_var(ret, result);
[[    else:]]
  // UNSUPPORTED: {{result_type.kind}} {{result_type.spelling}}
  (void)result;
  bool register_ok = FALSE;
[[    ]]
  if (!register_ok) {
    VERROR("Failed to register handle %d of type {{result_type.spelling}}.", ret);
    return FALSE;
  }
  return TRUE;
[[  else:]]
  {{fn.spelling}}({{', '.join('arg%d' % i for i in range(len(arguments)))}});
  return TRUE;
[[  ]]
}

[[]]

enum {
  NUM_FUNCTIONS = {{len(collector.functions)}}
};

typedef bool (*nb_command_func_t)(struct Message*, int);
static nb_command_func_t s_functions[] = {
  NULL,  /* TODO(binji): This should be errorif */
[[for i, fn in enumerate(collector.functions):]]
  nb_command_run_{{fn.spelling}},  /* {{i+1}} */
[[]]
};

bool nb_message_command_run(struct Message* message, int command_idx) {
  int function_idx = nb_message_command_function(message, command_idx);
  if (function_idx < 0 || function_idx > NUM_FUNCTIONS) {
    VERROR("Function id %d is out of range [0, %d].", function_idx, NUM_FUNCTIONS);
    return FALSE;
  }

  return s_functions[function_idx](message, command_idx);
}
