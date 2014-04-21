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
#include <math.h>
#include <stdlib.h>
#include <string.h>

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
  assert(s_handle_map_size <= new_capacity);
  s_handle_map = realloc(s_handle_map, sizeof(HandleMapPair) * new_capacity);
  s_handle_map_capacity = new_capacity;
  if (!s_handle_map) {
    ERROR("Out of memory");
    return FALSE;
  }
  return TRUE;
}

bool RegisterHandle(Handle handle, Type type, HandleValue value) {
  if (!s_handle_map) {
    if (!ResizeHandleMap(HANDLE_MAP_INITIAL_CAPACITY)) {
      return FALSE;
    }
  }

  if (s_handle_map_size == s_handle_map_capacity) {
    if (!ResizeHandleMap(s_handle_map_capacity * 2)) {
      return FALSE;
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
          VERROR("handle %d is already registered.", handle);
          return FALSE;
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
  return TRUE;
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
      return FALSE;
  }
}

bool GetHandle(Handle handle, HandleObject* out_handle_object) {
  // Binary search.
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

      *out_handle_object = s_handle_map[mid_ix].object;
      return TRUE;
    }
  }

  return FALSE;
}

#define TYPE_INT8_MIN (-0x80)
#define TYPE_INT8_MAX (0x7f)
#define TYPE_INT16_MIN (-0x8000)
#define TYPE_INT16_MAX (0x7fff)
#define TYPE_INT32_MIN (-0x80000000L)
#define TYPE_INT32_MAX (0x7fffffffL)
#define TYPE_INT64_MIN (-0x8000000000000000LL)
#define TYPE_INT64_MAX (0x7fffffffffffffffLL)
#define TYPE_UINT8_MIN (0)
#define TYPE_UINT8_MAX (0xff)
#define TYPE_UINT16_MIN (0)
#define TYPE_UINT16_MAX (0xffff)
#define TYPE_UINT32_MIN (0)
#define TYPE_UINT32_MAX (0xffffffffL)
#define TYPE_UINT64_MIN (0)
#define TYPE_UINT64_MAX (0xffffffffffffffffLL)
#define TYPE_FLOAT_MIN_24 (-0xffffffL)
#define TYPE_FLOAT_MAX_24 (0xffffffL)
#define TYPE_DOUBLE_MIN_53 (-0x1fffffffffffffLL)
#define TYPE_DOUBLE_MAX_53 (0x1fffffffffffffLL)

#define TYPE_INT8_FMT "%d"
#define TYPE_INT16_FMT "%d"
#define TYPE_INT32_FMT "%d"
#define TYPE_INT64_FMT "%lld"
#define TYPE_UINT8_FMT "%u"
#define TYPE_UINT16_FMT "%u"
#define TYPE_UINT32_FMT "%u"
#define TYPE_UINT64_FMT "%llu"
#define TYPE_FLOAT_FMT "%g"
#define TYPE_DOUBLE_FMT "%g"

#define TYPE_INT8_FIELD int8
#define TYPE_INT16_FIELD int16
#define TYPE_INT32_FIELD int32
#define TYPE_INT64_FIELD int64
#define TYPE_UINT8_FIELD uint8
#define TYPE_UINT16_FIELD uint16
#define TYPE_UINT32_FIELD uint32
#define TYPE_UINT64_FIELD uint64
#define TYPE_FLOAT_FIELD float32
#define TYPE_DOUBLE_FIELD float64

#define HOBJ_FIELD(type) (hobj.value.type##_FIELD)

#define TYPE_CASE(to_type, from_type) \
  case from_type: \
    *out_value = HOBJ_FIELD(from_type); \
    return TRUE  // no semicolon

#define DEFAULT_TYPE_CASE(to_type) \
  default: \
    VERROR("handle %d is of type %s. Expected %s.", handle, \
           TypeToString(hobj.type), TypeToString(to_type)); \
    return FALSE  // no semicolon

#define TYPE_CASE_TRUNC_GENERIC(to_type, from_type, check) \
  case from_type: \
    check(to_type, from_type); \
    *out_value = HOBJ_FIELD(from_type); \
    return TRUE  // no semicolon

#define TYPE_CASE_TRUNC(to_type, from_type) \
  TYPE_CASE_TRUNC_GENERIC(to_type, from_type, TRUNC_CHECK)

#define TYPE_CASE_TRUNC_MIN(to_type, from_type) \
  TYPE_CASE_TRUNC_GENERIC(to_type, from_type, TRUNC_CHECK_MIN)

#define TYPE_CASE_TRUNC_MAX(to_type, from_type) \
  TYPE_CASE_TRUNC_GENERIC(to_type, from_type, TRUNC_CHECK_MAX)

#define TYPE_CASE_TRUNC_FLOAT_TO_INT(to_type, from_type) \
  TYPE_CASE_TRUNC_GENERIC(to_type, from_type, TRUNC_CHECK_FLOAT_TO_INT)

#define TYPE_CASE_TRUNC_DOUBLE_TO_INT(to_type, from_type) \
  TYPE_CASE_TRUNC_GENERIC(to_type, from_type, TRUNC_CHECK_DOUBLE_TO_INT)

#define TYPE_CASE_TRUNC_INT_TO_FLOAT(to_type, from_type) \
  TYPE_CASE_TRUNC_GENERIC(to_type, from_type, TRUNC_CHECK_INT_TO_FLOAT)

#define TYPE_CASE_TRUNC_INT_TO_DOUBLE(to_type, from_type) \
  TYPE_CASE_TRUNC_GENERIC(to_type, from_type, TRUNC_CHECK_INT_TO_DOUBLE)

#define TRUNC_CHECK(to_type, from_type) \
  if (HOBJ_FIELD(from_type) < to_type##_MIN || \
      HOBJ_FIELD(from_type) > to_type##_MAX) { \
    TRUNC_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define TRUNC_CHECK_MAX(to_type, from_type) \
  if (HOBJ_FIELD(from_type) > to_type##_MAX) { \
    TRUNC_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define TRUNC_CHECK_MIN(to_type, from_type) \
  if (HOBJ_FIELD(from_type) < to_type##_MIN) { \
    TRUNC_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define TRUNC_CHECK_FLOAT_TO_INT(to_type, from_type) \
  if (HOBJ_FIELD(from_type) < to_type##_MIN || \
      HOBJ_FIELD(from_type) > to_type##_MAX || \
      HOBJ_FIELD(from_type) != rintf(HOBJ_FIELD(from_type))) { \
    TRUNC_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define TRUNC_CHECK_DOUBLE_TO_INT(to_type, from_type) \
  if (HOBJ_FIELD(from_type) < to_type##_MIN || \
      HOBJ_FIELD(from_type) > to_type##_MAX || \
      HOBJ_FIELD(from_type) != rint(HOBJ_FIELD(from_type))) { \
    TRUNC_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define TRUNC_CHECK_INT_TO_FLOAT(to_type, from_type) \
  if (HOBJ_FIELD(from_type) < TYPE_FLOAT_MIN_24 || \
      HOBJ_FIELD(from_type) > TYPE_FLOAT_MAX_24) { \
    TRUNC_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define TRUNC_CHECK_INT_TO_DOUBLE(to_type, from_type) \
  if (HOBJ_FIELD(from_type) < TYPE_DOUBLE_MIN_53 || \
      HOBJ_FIELD(from_type) > TYPE_DOUBLE_MAX_53) { \
    TRUNC_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define TRUNC_ERROR(to_type, from_type) \
  VERROR("handle %d(%s) with value " from_type##_FMT \
         " cannot be represented as %s.", handle, TypeToString(hobj.type), \
         HOBJ_FIELD(from_type), TypeToString(to_type))

bool GetHandleInt8(Handle handle, int8_t* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return FALSE;
  }

  switch (hobj.type) {
    TYPE_CASE(TYPE_INT8, TYPE_INT8);
    TYPE_CASE_TRUNC(TYPE_INT8, TYPE_INT16);
    TYPE_CASE_TRUNC(TYPE_INT8, TYPE_INT32);
    TYPE_CASE_TRUNC(TYPE_INT8, TYPE_INT64);
    TYPE_CASE_TRUNC_MAX(TYPE_INT8, TYPE_UINT8);
    TYPE_CASE_TRUNC_MAX(TYPE_INT8, TYPE_UINT16);
    TYPE_CASE_TRUNC_MAX(TYPE_INT8, TYPE_UINT32);
    TYPE_CASE_TRUNC_MAX(TYPE_INT8, TYPE_UINT64);
    TYPE_CASE_TRUNC_FLOAT_TO_INT(TYPE_INT8, TYPE_FLOAT);
    TYPE_CASE_TRUNC_DOUBLE_TO_INT(TYPE_INT8, TYPE_DOUBLE);
    DEFAULT_TYPE_CASE(TYPE_INT8);
  }
}

bool GetHandleUint8(Handle handle, uint8_t* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return FALSE;
  }

  switch (hobj.type) {
    TYPE_CASE_TRUNC_MIN(TYPE_UINT8, TYPE_INT8);
    TYPE_CASE_TRUNC(TYPE_UINT8, TYPE_INT16);
    TYPE_CASE_TRUNC(TYPE_UINT8, TYPE_INT32);
    TYPE_CASE_TRUNC(TYPE_UINT8, TYPE_INT64);
    TYPE_CASE(TYPE_UINT8, TYPE_UINT8);
    TYPE_CASE_TRUNC(TYPE_UINT8, TYPE_UINT16);
    TYPE_CASE_TRUNC(TYPE_UINT8, TYPE_UINT32);
    TYPE_CASE_TRUNC(TYPE_UINT8, TYPE_UINT64);
    TYPE_CASE_TRUNC_FLOAT_TO_INT(TYPE_UINT8, TYPE_FLOAT);
    TYPE_CASE_TRUNC_DOUBLE_TO_INT(TYPE_UINT8, TYPE_DOUBLE);
    DEFAULT_TYPE_CASE(TYPE_UINT8);
  }
}

bool GetHandleInt16(Handle handle, int16_t* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return FALSE;
  }

  switch (hobj.type) {
    TYPE_CASE(TYPE_INT16, TYPE_INT8);
    TYPE_CASE(TYPE_INT16, TYPE_INT16);
    TYPE_CASE_TRUNC(TYPE_INT16, TYPE_INT32);
    TYPE_CASE_TRUNC(TYPE_INT16, TYPE_INT64);
    TYPE_CASE(TYPE_INT16, TYPE_UINT8);
    TYPE_CASE_TRUNC_MAX(TYPE_INT16, TYPE_UINT16);
    TYPE_CASE_TRUNC(TYPE_INT16, TYPE_UINT32);
    TYPE_CASE_TRUNC(TYPE_INT16, TYPE_UINT64);
    TYPE_CASE_TRUNC_FLOAT_TO_INT(TYPE_INT16, TYPE_FLOAT);
    TYPE_CASE_TRUNC_DOUBLE_TO_INT(TYPE_INT16, TYPE_DOUBLE);
    DEFAULT_TYPE_CASE(TYPE_INT16);
  }
}

bool GetHandleUint16(Handle handle, uint16_t* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return FALSE;
  }

  switch (hobj.type) {
    TYPE_CASE_TRUNC_MIN(TYPE_UINT16, TYPE_INT8);
    TYPE_CASE_TRUNC_MIN(TYPE_UINT16, TYPE_INT16);
    TYPE_CASE_TRUNC(TYPE_UINT16, TYPE_INT32);
    TYPE_CASE_TRUNC(TYPE_UINT16, TYPE_INT64);
    TYPE_CASE(TYPE_UINT16, TYPE_UINT8);
    TYPE_CASE(TYPE_UINT16, TYPE_UINT16);
    TYPE_CASE_TRUNC(TYPE_UINT16, TYPE_UINT32);
    TYPE_CASE_TRUNC(TYPE_UINT16, TYPE_UINT64);
    TYPE_CASE_TRUNC_FLOAT_TO_INT(TYPE_UINT16, TYPE_FLOAT);
    TYPE_CASE_TRUNC_DOUBLE_TO_INT(TYPE_UINT16, TYPE_DOUBLE);
    DEFAULT_TYPE_CASE(TYPE_UINT16);
  }
}

bool GetHandleInt32(Handle handle, int32_t* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return FALSE;
  }

  switch (hobj.type) {
    TYPE_CASE(TYPE_INT32, TYPE_INT8);
    TYPE_CASE(TYPE_INT32, TYPE_INT16);
    TYPE_CASE(TYPE_INT32, TYPE_INT32);
    TYPE_CASE_TRUNC(TYPE_INT32, TYPE_INT64);
    TYPE_CASE(TYPE_INT32, TYPE_UINT8);
    TYPE_CASE(TYPE_INT32, TYPE_UINT16);
    TYPE_CASE_TRUNC_MAX(TYPE_INT32, TYPE_UINT32);
    TYPE_CASE_TRUNC(TYPE_INT32, TYPE_UINT64);
    TYPE_CASE_TRUNC_FLOAT_TO_INT(TYPE_INT32, TYPE_FLOAT);
    TYPE_CASE_TRUNC_DOUBLE_TO_INT(TYPE_INT32, TYPE_DOUBLE);
    DEFAULT_TYPE_CASE(TYPE_INT32);
  }
}

bool GetHandleUint32(Handle handle, uint32_t* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return FALSE;
  }

  switch (hobj.type) {
    TYPE_CASE_TRUNC_MIN(TYPE_UINT32, TYPE_INT8);
    TYPE_CASE_TRUNC_MIN(TYPE_UINT32, TYPE_INT16);
    TYPE_CASE_TRUNC_MIN(TYPE_UINT32, TYPE_INT32);
    TYPE_CASE_TRUNC(TYPE_UINT32, TYPE_INT64);
    TYPE_CASE(TYPE_UINT32, TYPE_UINT8);
    TYPE_CASE(TYPE_UINT32, TYPE_UINT16);
    TYPE_CASE(TYPE_UINT32, TYPE_UINT32);
    TYPE_CASE_TRUNC(TYPE_UINT32, TYPE_UINT64);
    TYPE_CASE_TRUNC_FLOAT_TO_INT(TYPE_UINT32, TYPE_FLOAT);
    TYPE_CASE_TRUNC_DOUBLE_TO_INT(TYPE_UINT32, TYPE_DOUBLE);
    DEFAULT_TYPE_CASE(TYPE_UINT32);
  }
}

bool GetHandleInt64(Handle handle, int64_t* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return FALSE;
  }

  switch (hobj.type) {
    TYPE_CASE(TYPE_INT64, TYPE_INT8);
    TYPE_CASE(TYPE_INT64, TYPE_INT16);
    TYPE_CASE(TYPE_INT64, TYPE_INT32);
    TYPE_CASE(TYPE_INT64, TYPE_INT64);
    TYPE_CASE(TYPE_INT64, TYPE_UINT8);
    TYPE_CASE(TYPE_INT64, TYPE_UINT16);
    TYPE_CASE(TYPE_INT64, TYPE_UINT32);
    TYPE_CASE_TRUNC_MAX(TYPE_INT64, TYPE_UINT64);
    TYPE_CASE_TRUNC_FLOAT_TO_INT(TYPE_INT64, TYPE_FLOAT);
    TYPE_CASE_TRUNC_DOUBLE_TO_INT(TYPE_INT64, TYPE_DOUBLE);
    DEFAULT_TYPE_CASE(TYPE_INT64);
  }
}

bool GetHandleUint64(Handle handle, uint64_t* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return FALSE;
  }

  switch (hobj.type) {
    TYPE_CASE_TRUNC_MIN(TYPE_UINT64, TYPE_INT8);
    TYPE_CASE_TRUNC_MIN(TYPE_UINT64, TYPE_INT16);
    TYPE_CASE_TRUNC_MIN(TYPE_UINT64, TYPE_INT32);
    TYPE_CASE_TRUNC_MIN(TYPE_UINT64, TYPE_INT64);
    TYPE_CASE(TYPE_UINT64, TYPE_UINT8);
    TYPE_CASE(TYPE_UINT64, TYPE_UINT16);
    TYPE_CASE(TYPE_UINT64, TYPE_UINT32);
    TYPE_CASE(TYPE_UINT64, TYPE_UINT64);
    TYPE_CASE_TRUNC_FLOAT_TO_INT(TYPE_UINT64, TYPE_FLOAT);
    TYPE_CASE_TRUNC_DOUBLE_TO_INT(TYPE_UINT64, TYPE_DOUBLE);
    DEFAULT_TYPE_CASE(TYPE_UINT64);
  }
}

bool GetHandleFloat(Handle handle, float* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return FALSE;
  }

  switch (hobj.type) {
    TYPE_CASE(TYPE_FLOAT, TYPE_INT8);
    TYPE_CASE(TYPE_FLOAT, TYPE_INT16);
    TYPE_CASE_TRUNC_INT_TO_FLOAT(TYPE_FLOAT, TYPE_INT32);
    TYPE_CASE_TRUNC_INT_TO_FLOAT(TYPE_FLOAT, TYPE_INT64);
    TYPE_CASE(TYPE_FLOAT, TYPE_UINT8);
    TYPE_CASE(TYPE_FLOAT, TYPE_UINT16);
    TYPE_CASE_TRUNC_INT_TO_FLOAT(TYPE_FLOAT, TYPE_UINT32);
    TYPE_CASE_TRUNC_INT_TO_FLOAT(TYPE_FLOAT, TYPE_UINT64);
    TYPE_CASE(TYPE_FLOAT, TYPE_FLOAT);
    TYPE_CASE(TYPE_FLOAT, TYPE_DOUBLE);
    DEFAULT_TYPE_CASE(TYPE_FLOAT);
  }
}

bool GetHandleDouble(Handle handle, double* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return FALSE;
  }

  switch (hobj.type) {
    TYPE_CASE(TYPE_DOUBLE, TYPE_INT8);
    TYPE_CASE(TYPE_DOUBLE, TYPE_INT16);
    TYPE_CASE(TYPE_DOUBLE, TYPE_INT32);
    TYPE_CASE_TRUNC_INT_TO_DOUBLE(TYPE_DOUBLE, TYPE_INT64);
    TYPE_CASE(TYPE_DOUBLE, TYPE_UINT8);
    TYPE_CASE(TYPE_DOUBLE, TYPE_UINT16);
    TYPE_CASE(TYPE_DOUBLE, TYPE_UINT32);
    TYPE_CASE_TRUNC_INT_TO_DOUBLE(TYPE_DOUBLE, TYPE_UINT64);
    TYPE_CASE(TYPE_DOUBLE, TYPE_FLOAT);
    TYPE_CASE(TYPE_DOUBLE, TYPE_DOUBLE);
    DEFAULT_TYPE_CASE(TYPE_DOUBLE);
  }
}

bool GetHandleVoidp(Handle handle, void** out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return FALSE;
  }

  if (hobj.type != TYPE_VOID_P) {
    VERROR("handle %d is of type %s. Expected %s.", handle,
          TypeToString(hobj.type), TypeToString(TYPE_VOID_P));
    return FALSE;
  }

  *out_value = hobj.value.voidp;
  return TRUE;
}

bool GetHandleVar(Handle handle, struct PP_Var* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return FALSE;
  }

  if (hobj.type != TYPE_ARRAY_BUFFER &&
      hobj.type != TYPE_ARRAY &&
      hobj.type != TYPE_DICTIONARY) {
    VERROR("handle %d is of type %s. Expected %s.", handle,
          TypeToString(hobj.type), TypeToString(hobj.type));
    return FALSE;
  }

  *out_value = hobj.value.var;
  return TRUE;
}

void DestroyHandle(Handle handle) {
  HandleMapPair* pair = NULL;


  // Binary search.
  size_t lo_ix = 0;  // Inclusive
  size_t hi_ix = s_handle_map_size;  // Exclusive
  size_t mid_ix;

  while (lo_ix < hi_ix) {
    mid_ix = (lo_ix + hi_ix) / 2;
    Handle mid_handle = s_handle_map[mid_ix].handle;
    if (handle > mid_handle) {
      lo_ix = mid_ix + 1;
    } else if (handle < mid_handle) {
      hi_ix = mid_ix;
    } else {
      pair = &s_handle_map[mid_ix];
      break;
    }
  }

  if (pair == NULL) {
    VERROR("Destroying handle %d, but it doesn't exist.", handle);
    return;
  }

  switch (pair->object.type) {
    case TYPE_ARRAY_BUFFER:
    case TYPE_ARRAY:
    case TYPE_DICTIONARY:
      ReleaseVar(&pair->object.value.var);
      break;
    default:
      break;
  }

  size_t remove_ix = mid_ix;
  if (remove_ix + 1 < s_handle_map_size) {
    memmove(&s_handle_map[remove_ix], &s_handle_map[remove_ix + 1],
            sizeof(HandleMapPair) * (s_handle_map_size - (remove_ix + 1)));
  }
  s_handle_map_size--;
}

void DestroyHandles(Handle* handles, int32_t handle_count) {
  // TODO(binji): optimize
  int32_t i;
  for (i = 0; i < handle_count; ++i) {
    Handle handle = handles[i];
    DestroyHandle(handle);
  }
}

bool HandleToVar(Handle handle, struct PP_Var* var) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    return FALSE;
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
      return FALSE;
  }

  return TRUE;
}
