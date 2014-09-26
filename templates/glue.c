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

/* DO NOT EDIT, this file is auto-generated from //templates/glue.gen.c */

#define NB_ONE_FILE

{{IncludeCFile('bool.h')}}
{{IncludeCFile('error.h')}}
{{IncludeCFile('handle.h')}}
{{IncludeCFile('interfaces.h')}}
{{IncludeCFile('message.h')}}
{{IncludeCFile('queue.h')}}
{{IncludeCFile('run.h')}}
{{IncludeCFile('type.h')}}
{{IncludeCFile('var.h')}}

{{IncludeCFile('handle.c')}}
{{IncludeCFile('interfaces.c')}}
{{IncludeCFile('message.c')}}
{{IncludeCFile('run.c')}}
{{IncludeCFile('type.c')}}
{{IncludeCFile('var.c')}}

#ifndef NB_NO_APP
{{IncludeCFile('queue.c')}}
{{IncludeCFile('app.c')}}
#endif

/* ========================================================================== */

#include "{{filename}}"

[[for fn in collector.functions:]]
static bool nb_command_run_{{fn.spelling}}(struct Message* message, int command_idx) {
[[  arguments = list(fn.type.argument_types())]]
  int arg_count = nb_message_command_arg_count(message, command_idx);
  if (arg_count != {{len(arguments)}}) {
    return FALSE;
  }
[[  for i, arg in enumerate(arguments):]]
[[    arg = arg.get_canonical()]]
  Handle handle{{i}} = nb_message_command_arg(message, command_idx, {{i}});
[[    if arg.kind == TypeKind.POINTER:]]
[[      pointee = arg.get_pointee()]]
[[      if pointee.kind in (TypeKind.CHAR_S, TypeKind.CHAR_U):]]
  char* arg{{i}};
  if (!nb_handle_get_charp(handle{{i}}, &arg{{i}})) {
    // print error.
    return FALSE;
  }
[[      elif pointee.kind == TypeKind.VOID:]]
  void* arg{{i}};
  if (!nb_handle_get_voidp(handle{{i}}, &arg{{i}})) {
    // print error.
    return FALSE;
  }
[[      elif pointee.kind == TypeKind.FUNCTIONPROTO:]]
  // UNSUPPORTED: {{arg.kind}} {{arg.spelling}}
  void* arg{{i}} = NULL;
[[      else:]]
  void* arg{{i}}x;
  if (!nb_handle_get_voidp(handle{{i}}, &arg{{i}}x)) {
    // print error.
    return FALSE;
  }
  {{arg.spelling}}* arg{{i}} = ({{arg.spelling}}*) arg{{i}}x;
[[    elif arg.kind == TypeKind.LONG:]]
  int32_t arg{{i}}x;
  if (!nb_handle_get_int32(handle{{i}}, &arg{{i}}x)) {
    // print error.
    return FALSE;
  }
  long arg{{i}} = (long) arg{{i}}x;
[[    elif arg.kind == TypeKind.ULONG:]]
  uint32_t arg{{i}}x;
  if (!nb_handle_get_uint32(handle{{i}}, &arg{{i}}x)) {
    // print error.
    return FALSE;
  }
  unsigned long arg{{i}} = (unsigned long) arg{{i}}x;
[[    elif arg.kind == TypeKind.LONGLONG:]]
  int64_t arg{{i}};
  if (!nb_handle_get_int64(handle{{i}}, &arg{{i}})) {
    // print error.
    return FALSE;
  }
[[    elif arg.kind == TypeKind.ULONGLONG:]]
  uint64_t arg{{i}};
  if (!nb_handle_get_uint64(handle{{i}}, &arg{{i}})) {
    // print error.
    return FALSE;
  }
[[    elif arg.kind in (TypeKind.INT, TypeKind.SHORT, TypeKind.SCHAR, TypeKind.CHAR_S):]]
  int32_t arg{{i}};
  if (!nb_handle_get_int32(handle{{i}}, &arg{{i}})) {
    // print error.
    return FALSE;
  }
[[    elif arg.kind in (TypeKind.UINT, TypeKind.USHORT, TypeKind.UCHAR, TypeKind.CHAR_U):]]
  uint32_t arg{{i}};
  if (!nb_handle_get_uint32(handle{{i}}, &arg{{i}})) {
    // print error.
    return FALSE;
  }
[[    elif arg.kind == TypeKind.FLOAT:]]
  float arg{{i}};
  if (!nb_handle_get_float(handle{{i}}, &arg{{i}})) {
    // print error.
    return FALSE;
  }
[[    elif arg.kind == TypeKind.DOUBLE:]]
  double arg{{i}};
  if (!nb_handle_get_double(handle{{i}}, &arg{{i}})) {
    // print error.
    return FALSE;
  }
[[    elif arg.kind == TypeKind.ENUM:]]
  int32_t arg{{i}}x;
  if (!nb_handle_get_int32(handle{{i}}, &arg{{i}}x)) {
    // print error.
    return FALSE;
  }
  {{arg.spelling}} arg{{i}} = ({{arg.spelling}}) arg{{i}}x;
[[    elif arg.kind == TypeKind.CONSTANTARRAY:]]
  // UNSUPPORTED: {{arg.kind}} {{arg.spelling}}
  void* arg{{i}} = NULL;
[[    else:]]
  // UNSUPPORTED: {{arg.kind}} {{arg.spelling}}
[[  ]]
[[  if fn.type.is_function_variadic():]]
  // UNSUPPORTED: variadic function.
[[  ]]
[[  result_type = fn.type.get_result().get_canonical()]]
[[  if result_type.kind != TypeKind.VOID:]]
  if (!nb_message_command_has_ret(message, command_idx)) {
    // print error.
    return FALSE;
  }
  Handle ret = nb_message_command_ret(message, command_idx);
  {{result_type.spelling}} result = {{fn.spelling}}({{', '.join('arg%d' % i for i in range(len(arguments)))}});
[[    if result_type.kind in (TypeKind.SCHAR, TypeKind.CHAR_S):]]
  return nb_handle_register_int8(ret, result);
[[    elif result_type.kind in (TypeKind.UCHAR, TypeKind.CHAR_U):]]
  return nb_handle_register_uint8(ret, result);
[[    elif result_type.kind == TypeKind.SHORT:]]
  return nb_handle_register_int16(ret, result);
[[    elif result_type.kind == TypeKind.USHORT:]]
  return nb_handle_register_uint16(ret, result);
[[    elif result_type.kind == TypeKind.INT:]]
  return nb_handle_register_int32(ret, result);
[[    elif result_type.kind == TypeKind.UINT:]]
  return nb_handle_register_uint32(ret, result);
[[    elif result_type.kind == TypeKind.LONG:]]
  return nb_handle_register_int32(ret, (int32_t)result);
[[    elif result_type.kind == TypeKind.ULONG:]]
  return nb_handle_register_uint32(ret, (uint32_t)result);
[[    elif result_type.kind == TypeKind.LONGLONG:]]
  return nb_handle_register_int64(ret, result);
[[    elif result_type.kind == TypeKind.ULONGLONG:]]
  return nb_handle_register_uint64(ret, result);
[[    elif result_type.kind == TypeKind.FLOAT:]]
  return nb_handle_register_float(ret, result);
[[    elif result_type.kind == TypeKind.DOUBLE:]]
  return nb_handle_register_double(ret, result);
[[    elif result_type.kind == TypeKind.POINTER:]]
  return nb_handle_register_voidp(ret, result);
[[    else:]]
  // UNSUPPORTED: {{result_type.kind}} {{result_type.spelling}}
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
[[for fn in collector.functions:]]
  nb_command_run_{{fn.spelling}},
[[]]
};

bool nb_message_command_run(struct Message* message, int command_idx) {
  int function_idx = nb_message_command_function(message, command_idx);
  if (function_idx < 0 || function_idx > NUM_FUNCTIONS) {
    return FALSE;
  }

  return s_functions[function_idx](message, command_idx);
}
