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

[[for fn in collector.functions:]]
static void Handle_{{fn.spelling}}(Command* command) {
[[  arguments = list(fn.type.argument_types())]]
[[  for i, arg in enumerate(arguments):]]
[[    arg = arg.get_canonical()]]
[[    if arg.kind == TypeKind.POINTER:]]
[[      pointee = arg.get_pointee()]]
[[      if pointee.kind in (TypeKind.CHAR_S, TypeKind.CHAR_U):]]
  ARG_CHARP({{i}});
[[      elif pointee.kind == TypeKind.VOID:]]
  ARG_VOIDP({{i}});
[[      elif pointee.kind == TypeKind.FUNCTIONPROTO:]]
  // UNSUPPORTED: {{arg.kind}} {{arg.spelling}}
  void* arg{{i}} = NULL;
[[      else:]]
  ARG_VOIDP_CAST({{i}}, {{arg.spelling}});
[[    elif arg.kind == TypeKind.LONG:]]
  ARG_INT_CAST({{i}}, long);
[[    elif arg.kind == TypeKind.ULONG:]]
  ARG_UINT_CAST({{i}}, unsigned long);
[[    elif arg.kind == TypeKind.LONGLONG:]]
  ARG_INT64({{i}});
[[    elif arg.kind == TypeKind.ULONGLONG:]]
  ARG_UINT64({{i}});
[[    elif arg.kind in (TypeKind.INT, TypeKind.SHORT, TypeKind.SCHAR, TypeKind.CHAR_S):]]
  ARG_INT({{i}});
[[    elif arg.kind in (TypeKind.UINT, TypeKind.USHORT, TypeKind.UCHAR, TypeKind.CHAR_U):]]
  ARG_UINT({{i}});
[[    elif arg.kind == TypeKind.ULONG:]]
  ARG_UINT_CAST({{i}}, unsigned long);
[[    elif arg.kind == TypeKind.FLOAT:]]
  ARG_FLOAT32({{i}});
[[    elif arg.kind == TypeKind.DOUBLE:]]
  ARG_FLOAT64({{i}});
[[    elif arg.kind == TypeKind.ENUM:]]
  ARG_INT_CAST({{i}}, {{arg.spelling}});
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
  {{result_type.spelling}} result = {{fn.spelling}}({{', '.join('arg%d' % i for i in range(len(arguments)))}});
[[    if result_type.kind in (TypeKind.SCHAR, TypeKind.CHAR_S):]]
  RegisterHandleInt8(command->ret_handle, result);
[[    elif result_type.kind in (TypeKind.UCHAR, TypeKind.CHAR_U):]]
  RegisterHandleUint8(command->ret_handle, result);
[[    elif result_type.kind == TypeKind.SHORT:]]
  RegisterHandleInt16(command->ret_handle, result);
[[    elif result_type.kind == TypeKind.USHORT:]]
  RegisterHandleUint16(command->ret_handle, result);
[[    elif result_type.kind == TypeKind.INT:]]
  RegisterHandleInt32(command->ret_handle, result);
[[    elif result_type.kind == TypeKind.UINT:]]
  RegisterHandleUint32(command->ret_handle, result);
[[    elif result_type.kind == TypeKind.LONG:]]
  RegisterHandleInt32(command->ret_handle, (int32_t)result);
[[    elif result_type.kind == TypeKind.ULONG:]]
  RegisterHandleUint32(command->ret_handle, (uint32_t)result);
[[    elif result_type.kind == TypeKind.LONGLONG:]]
  RegisterHandleInt64(command->ret_handle, result);
[[    elif result_type.kind == TypeKind.ULONGLONG:]]
  RegisterHandleUint64(command->ret_handle, result);
[[    elif result_type.kind == TypeKind.FLOAT:]]
  RegisterHandleFloat(command->ret_handle, result);
[[    elif result_type.kind == TypeKind.DOUBLE:]]
  RegisterHandleDouble(command->ret_handle, result);
[[    elif result_type.kind == TypeKind.POINTER:]]
  RegisterHandleVoidp(command->ret_handle, result);
[[    else:]]
  // UNSUPPORTED: {{result_type.kind}} {{result_type.spelling}}
  RegisterHandleVoidp(command->ret_handle, NULL);
[[  else:]]
  {{fn.spelling}}({{', '.join('arg%d' % i for i in range(len(arguments)))}});
[[  ]]
}
[[]]

enum {
  NUM_FUNCTIONS = {{len(collector.functions)}};
};

static HandleFunc g_FuncMap[] = {
[[for fn in collector.functions:]]
  Handle_{{fn.spelling}},
[[]]
};

static bool HandleCommand(Command* command) {
  if (command->command < 0 || command->command >= NUM_FUNCTIONS) {
    return FALSE;
  }

  HandleFunc func = &g_FuncMap[command->command];
  func(command);
  return TRUE;
}
