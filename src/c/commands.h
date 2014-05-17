// Copyright 2014 Ben Smith. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef COMMANDS_H_
#define COMMANDS_H_

#include "bool.h"
#include "message.h"


bool HandleBuiltinCommand(Command* command);
bool GetArgVoidp(Command* command, int32_t index, void** out_value);
bool GetArgInt32(Command* command, int32_t index, int32_t* out_value);
bool GetArgUint32(Command* command, int32_t index, uint32_t* out_value);
bool GetArgVar(Command* command, int32_t index, struct PP_Var* out_value);


#define TYPE_CHECK(expected) \
  VERROR_IF(command->type == expected, \
            "Type mismatch. Expected %s. Got %s.", \
            TypeToString(expected), \
            TypeToString(command->type))

#define TYPE_FAIL \
  VERROR("Type didn't match any types. Got %s.", TypeToString(command->type))

#define CMD_VERROR(fmt, ...) \
  VERROR("%s: " fmt, command->command, __VA_ARGS__)

#define CMD_ERROR(msg) \
  VERROR("%s: " msg, command->command)

#define ARG_VOIDP(index) \
  void* arg##index; \
  if (!GetArgVoidp(command, index, &arg##index)) return

#define ARG_VOIDP_CAST(index, type) \
  void* arg##index##_voidp; \
  if (!GetArgVoidp(command, index, &arg##index##_voidp)) return; \
  type arg##index = (type)arg##index##_voidp

#define ARG_INT(index) \
  int32_t arg##index; \
  if (!GetArgInt32(command, index, &arg##index)) return

#define ARG_INT_CAST(index, type) \
  int32_t arg##index##_int; \
  if (!GetArgInt32(command, index, &arg##index##_int)) return; \
  type arg##index = (type)arg##index##_int

#define ARG_UINT(index) \
  uint32_t arg##index; \
  if (!GetArgUint32(command, index, &arg##index)) return

#define ARG_UINT_CAST(index, type) \
  uint32_t arg##index##_uint; \
  if (!GetArgUint32(command, index, &arg##index##_uint)) return; \
  type arg##index = (type)arg##index##_uint

#define ARG_VAR(index) \
  struct PP_Var arg##index; \
  if (!GetArgVar(command, index, &arg##index)) return


#endif  // COMMANDS_H_
