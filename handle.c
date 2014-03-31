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

#include "handle.h"

#include <assert.h>

#include <ppapi/c/pp_var.h>

#include "error.h"
#include "var.h"

#define HANDLE_MAP_INITIAL_CAPACITY 16

typedef struct {
  Handle handle;
  HandleObject object;
} HandleMapPair;

static HandleMapPair* s_handle_map = NULL;
static size_t s_handle_map_size = 0;
static size_t s_handle_map_capacity = 0;

static bool ResizeHandleMap(size_t new_capacity) {
  assert(s_handle_map_size <= new_capacity)
  HandleMapPair* new_map = malloc(sizeof(HandleMapPair) * new_capacity);
  if (!new_map) {
    return false;
  }

  memcpy(new_map, s_handle_map, sizeof(HandleMapPair) * s_handle_map_size);
  free(s_handle_map);
  s_handle_map = new_map;
  return true;
}

bool RegisterHandle(Handle handle, Type type, HandleValue value) {
  if (!s_handle_map) {
    s_handle_map = malloc(sizeof(HandleMapPair) * HANDLE_MAP_INITIAL_CAPACITY);
    if (!s_handle_map) {
      ERROR("Out of memory");
      return false;
    }
    s_handle_map_capacity = HANDLE_MAP_INITIAL_CAPACITY;
  }

  if (s_handle_map_size == s_handle_map_capacity) {
    if (!ResizeHandleMap(s_handle_map_capacity * 2)) {
      ERROR("Out of memory");
      return false;
    }
  }

  HandleMapPair* pair = NULL;

  if (s_handle_map_size == 0) {
    assert(s_handle_map_capacity > 0);
    pair = &s_handle_map[0];
  } else {
    // Fast case, the new handle is larger than all other handles.
    if (handle > s_handle_map[s_handle_map_size - 1].handle) {
      pair = &s_handle_map[s_handle_map_size];
    } else {
      // Binary search to find the insertion point.
      size_t lo_ix = 0;  // Inclusive
      size_t hi_ix = s_handle_map_size;  // Exclusive
      while (lo_ix < hi_ix) {
        size_t mid_ix = (lo_ix + hi_ix) / 2;
        Handle mid_handle = s_handle_map[mid_ix].handle;
        if (handle > mid_handle) {
          lo_ix = mid_ix + 1;
        } else if (handle < mid_handle) {
          hi_ix = mid_ix;
        } else {
          VERROR("handle %d is already registered.\n", handle);
          return false;
        }
      }

      // Move everything after the insertion point down.
      size_t insert_ix = lo_ix;
      if (insert_ix < s_handle_map_size) {
        memmove(&s_handle_map[insert_ix + 1], &s_handle_map[insert_ix],
                sizeof(HandleMapPair) * (s_handle_map_size - insert_ix));
      }

      pair = &s_handle_map[insert_ix];
    }
  }

  pair->handle = handle;
  pair->object.type = type;
  pair->object.value = value;
  s_handle_map_size++;
  return true;
}

bool RegisterHandleInt8(Handle handle, int8_t value) {
  HandleValue hval;
  hval.int8 = value;
  return RegisterHandle(handle, TYPE_INT8, hval);
}

bool RegisterHandleUint8(Handle handle, uint8_t value) {
  HandleValue hval;
  hval.uint8 = value;
  return RegisterHandle(handle, TYPE_INT8, hval);
}

bool RegisterHandleInt16(Handle handle, int16_t value) {
  HandleValue hval;
  hval.int16 = value;
  return RegisterHandle(handle, TYPE_INT16, hval);
}

bool RegisterHandleUint16(Handle handle, uint16_t value) {
  HandleValue hval;
  hval.uint16 = value;
  return RegisterHandle(handle, TYPE_UINT16, hval);
}

bool RegisterHandleInt32(Handle handle, int32_t value) {
  HandleValue hval;
  hval.int32 = value;
  return RegisterHandle(handle, TYPE_INT32, hval);
}

bool RegisterHandleUint32(Handle handle, uint32_t value) {
  HandleValue hval;
  hval.uint32 = value;
  return RegisterHandle(handle, TYPE_UINT32, hval);
}

bool RegisterHandleInt64(Handle handle, int64_t value) {
  HandleValue hval;
  hval.int64 = value;
  return RegisterHandle(handle, TYPE_INT64, hval);
}

bool RegisterHandleUint64(Handle handle, uint64_t value) {
  HandleValue hval;
  hval.uint64 = value;
  return RegisterHandle(handle, TYPE_UINT64, hval);
}

bool RegisterHandleFloat(Handle handle, float value) {
  HandleValue hval;
  hval.float32 = value;
  return RegisterHandle(handle, TYPE_FLOAT, hval);
}

bool RegisterHandleDouble(Handle handle, double value) {
  HandleValue hval;
  hval.float64 = value;
  return RegisterHandle(handle, TYPE_DOUBLE, hval);
}

bool RegisterHandleVoidp(Handle handle, void* value) {
  HandleValue hval;
  hval.voidp = value;
  return RegisterHandle(handle, TYPE_VOID_P, hval);
}

bool RegisterHandleVar(Handle handle, struct PP_Var value) {
  HandleValue hval;
  hval.var = value;
  AddRefVar(&hval.var);

  switch (value.type) {
    case PP_VARTYPE_ARRAY_BUFFER:
      return RegisterHandle(handle, TYPE_ARRAY_BUFFER, hval);
    case PP_VARTYPE_ARRAY:
      return RegisterHandle(handle, TYPE_ARRAY, hval);
    case PP_VARTYPE_DICTIONARY:
      return RegisterHandle(handle, TYPE_DICTIONARY, hval);
    default:
      return false;
  }
}

bool GetHandle(Handle handle, HandleObject* out_handle_object) {
  HandleMap::iterator iter = g_handle_map.find(handle);
  if (iter == g_handle_map.end()) {
    return false;
  }

  *out_handle_object = iter->second;
  return true;
}

bool GetHandleInt8(Handle handle, int8_t* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return false;
  }

  if (hobj.type != TYPE_INT8) {
    VERROR("handle %d is of type %s. Expected %s.\n", handle,
          TypeToString(hobj.type), TypeToString(TYPE_INT8));
    return false;
  }

  *out_value = hobj.value.int8;
  return true;
}

bool GetHandleUint8(Handle handle, uint8_t* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return false;
  }

  if (hobj.type != TYPE_UINT8) {
    VERROR("handle %d is of type %s. Expected %s.\n", handle,
          TypeToString(hobj.type), TypeToString(TYPE_UINT8));
    return false;
  }

  *out_value = hobj.value.uint8;
  return true;
}

bool GetHandleInt16(Handle handle, int16_t* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return false;
  }

  if (hobj.type != TYPE_INT16) {
    VERROR("handle %d is of type %s. Expected %s.\n", handle,
          TypeToString(hobj.type), TypeToString(TYPE_INT16));
    return false;
  }

  *out_value = hobj.value.int16;
  return true;
}

bool GetHandleUint16(Handle handle, uint16_t* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return false;
  }

  if (hobj.type != TYPE_UINT16) {
    VERROR("handle %d is of type %s. Expected %s.\n", handle,
          TypeToString(hobj.type), TypeToString(TYPE_UINT16));
    return false;
  }

  *out_value = hobj.value.uint16;
  return true;
}

bool GetHandleInt32(Handle handle, int32_t* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return false;
  }

  if (hobj.type != TYPE_INT32) {
    VERROR("handle %d is of type %s. Expected %s.\n", handle,
          TypeToString(hobj.type), TypeToString(TYPE_INT32));
    return false;
  }

  *out_value = hobj.value.int32;
  return true;
}

bool GetHandleUint32(Handle handle, uint32_t* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return false;
  }

  if (hobj.type != TYPE_UINT32) {
    VERROR("handle %d is of type %s. Expected %s.\n", handle,
          TypeToString(hobj.type), TypeToString(TYPE_UINT32));
    return false;
  }

  *out_value = hobj.value.uint32;
  return true;
}

bool GetHandleInt64(Handle handle, int64_t* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return false;
  }

  if (hobj.type != TYPE_INT64) {
    VERROR("handle %d is of type %s. Expected %s.\n", handle,
          TypeToString(hobj.type), TypeToString(TYPE_INT64));
    return false;
  }

  *out_value = hobj.value.int64;
  return true;
}

bool GetHandleUint64(Handle handle, uint64_t* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return false;
  }

  if (hobj.type != TYPE_UINT64) {
    VERROR("handle %d is of type %s. Expected %s.\n", handle,
          TypeToString(hobj.type), TypeToString(TYPE_UINT64));
    return false;
  }

  *out_value = hobj.value.uint64;
  return true;
}

bool GetHandleFloat(Handle handle, float* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return false;
  }

  if (hobj.type != TYPE_FLOAT) {
    VERROR("handle %d is of type %s. Expected %s.\n", handle,
          TypeToString(hobj.type), TypeToString(TYPE_FLOAT));
    return false;
  }

  *out_value = hobj.value.float32;
  return true;
}

bool GetHandleDouble(Handle handle, double* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return false;
  }

  if (hobj.type != TYPE_DOUBLE) {
    VERROR("handle %d is of type %s. Expected %s.\n", handle,
          TypeToString(hobj.type), TypeToString(TYPE_DOUBLE));
    return false;
  }

  *out_value = hobj.value.float64;
  return true;
}

bool GetHandleVoidp(Handle handle, void** out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return false;
  }

  if (hobj.type != TYPE_VOID_P) {
    VERROR("handle %d is of type %s. Expected %s.\n", handle,
          TypeToString(hobj.type), TypeToString(TYPE_VOID_P));
    return false;
  }

  *out_value = hobj.value.voidp;
  return true;
}

bool GetHandleVar(Handle handle, struct PP_Var* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return false;
  }

  if (hobj.type != TYPE_ARRAY_BUFFER &&
      hobj.type != TYPE_ARRAY &&
      hobj.type != TYPE_DICTIONARY) {
    VERROR("handle %d is of type %s. Expected %s.\n", handle,
          TypeToString(hobj.type), TypeToString(hobj.type));
    return false;
  }

  *out_value = hobj.value.var;
  return true;
}

void DestroyHandle(Handle handle) {
  HandleMap::iterator iter = g_handle_map.find(handle);
  if (iter == g_handle_map.end()) {
    VERROR("handle %d doesn't exist.\n", handle);
    return;
  }

  HandleObject& hobj = iter->second;

  switch (hobj.type) {
    case TYPE_ARRAY_BUFFER:
    case TYPE_ARRAY:
    case TYPE_DICTIONARY:
      ReleaseVar(&hobj.value.var);
      break;
    default:
      break;
  }

  g_handle_map.erase(iter);
}

bool HandleToVar(Handle handle, struct PP_Var* var) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return false;
  }

  switch (hobj.type) {
    case TYPE_INT8:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hobj.value.int8;
      break;
    case TYPE_UINT8:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hobj.value.uint8;
      break;
    case TYPE_INT16:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hobj.value.int16;
      break;
    case TYPE_UINT16:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hobj.value.uint16;
      break;
    case TYPE_INT32:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hobj.value.int32;
      break;
    case TYPE_UINT32:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hobj.value.uint32;
      break;
    case TYPE_FLOAT:
      var->type = PP_VARTYPE_DOUBLE;
      var->value.as_int = hobj.value.float32;
      break;
    case TYPE_DOUBLE:
      var->type = PP_VARTYPE_DOUBLE;
      var->value.as_int = hobj.value.float64;
      break;
    case TYPE_ARRAY_BUFFER:
    case TYPE_ARRAY:
    case TYPE_DICTIONARY:
      *var = hobj.value.var;
      AddRefVar(var);
      break;
    default:
      VERROR("Don't know how to convert handle %d with type %s to var", handle,
             TypeToString(hobj.type));
      return false;
  }

  return true;
}
