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

#ifndef NB_ONE_FILE
#include "handle.h"
#endif

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ppapi/c/pp_var.h>

#ifndef NB_ONE_FILE
#include "error.h"
#include "interfaces.h"
#include "var.h"
#endif

#define HANDLE_MAP_INITIAL_CAPACITY 16

union HandleValue {
  int8_t int8;
  uint8_t uint8;
  int16_t int16;
  uint16_t uint16;
  int32_t int32;
  uint32_t uint32;
  int64_t int64;
  uint64_t uint64;
  float float32;
  double float64;
  void* voidp;
  struct PP_Var var;
} HandleValue;

struct HandleObject {
  Type type;
  union HandleValue value;
  /* PP_Var strings are not guaranteed to be NULL-terminated, so if we want to
   * use it as a C string, we have to allocate space for a NULL and remember to
   * free it later.
   *
   * This field will be non-NULL when type == TYPE_VAR and
   * nb_handle_get_charp() has been called. The memory will be free'd in
   * DestroyHandle.
   */
  char* string_value;
};


struct HandleMapPair {
  Handle handle;
  struct HandleObject object;
};

/* TODO(binji): use hashmap instead of sorted array. */
static struct HandleMapPair* s_handle_map = NULL;
static size_t s_handle_map_size = 0;
static size_t s_handle_map_capacity = 0;

static bool resize_handle_map(size_t new_capacity) {
  assert(s_handle_map_size <= new_capacity);
  s_handle_map = realloc(s_handle_map,
                         sizeof(struct HandleMapPair) * new_capacity);
  s_handle_map_capacity = new_capacity;
  if (!s_handle_map) {
    ERROR("Out of memory");
    return FALSE;
  }
  return TRUE;
}

static bool register_handle(Handle handle, Type type, union HandleValue value) {
  if (!s_handle_map) {
    if (!resize_handle_map(HANDLE_MAP_INITIAL_CAPACITY)) {
      return FALSE;
    }
  }

  if (s_handle_map_size == s_handle_map_capacity) {
    if (!resize_handle_map(s_handle_map_capacity * 2)) {
      return FALSE;
    }
  }

  struct HandleMapPair* pair = NULL;

  if (s_handle_map_size == 0) {
    assert(s_handle_map_capacity > 0);
    pair = &s_handle_map[0];
  } else {
    /* Fast case, the new handle is larger than all other handles. */
    if (handle > s_handle_map[s_handle_map_size - 1].handle) {
      pair = &s_handle_map[s_handle_map_size];
    } else {
      /* Binary search to find the insertion point. */
      size_t lo_ix = 0;  /* Inclusive */
      size_t hi_ix = s_handle_map_size;  /* Exclusive */
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

      /* Move everything after the insertion point down. */
      size_t insert_ix = lo_ix;
      if (insert_ix < s_handle_map_size) {
        memmove(&s_handle_map[insert_ix + 1], &s_handle_map[insert_ix],
                sizeof(struct HandleMapPair) * (s_handle_map_size - insert_ix));
      }

      pair = &s_handle_map[insert_ix];
    }
  }

  pair->handle = handle;
  pair->object.type = type;
  pair->object.value = value;
  pair->object.string_value = NULL;
  s_handle_map_size++;
  return TRUE;
}

int32_t nb_handle_count(void) {
  return s_handle_map_size;
}

bool nb_handle_register_int8(Handle handle, int8_t value) {
  union HandleValue hval;
  hval.int8 = value;
  return register_handle(handle, TYPE_INT8, hval);
}

bool nb_handle_register_uint8(Handle handle, uint8_t value) {
  union HandleValue hval;
  hval.uint8 = value;
  return register_handle(handle, TYPE_UINT8, hval);
}

bool nb_handle_register_int16(Handle handle, int16_t value) {
  union HandleValue hval;
  hval.int16 = value;
  return register_handle(handle, TYPE_INT16, hval);
}

bool nb_handle_register_uint16(Handle handle, uint16_t value) {
  union HandleValue hval;
  hval.uint16 = value;
  return register_handle(handle, TYPE_UINT16, hval);
}

bool nb_handle_register_int32(Handle handle, int32_t value) {
  union HandleValue hval;
  hval.int32 = value;
  return register_handle(handle, TYPE_INT32, hval);
}

bool nb_handle_register_uint32(Handle handle, uint32_t value) {
  union HandleValue hval;
  hval.uint32 = value;
  return register_handle(handle, TYPE_UINT32, hval);
}

bool nb_handle_register_int64(Handle handle, int64_t value) {
  union HandleValue hval;
  hval.int64 = value;
  return register_handle(handle, TYPE_INT64, hval);
}

bool nb_handle_register_uint64(Handle handle, uint64_t value) {
  union HandleValue hval;
  hval.uint64 = value;
  return register_handle(handle, TYPE_UINT64, hval);
}

bool nb_handle_register_float(Handle handle, float value) {
  union HandleValue hval;
  hval.float32 = value;
  return register_handle(handle, TYPE_FLOAT, hval);
}

bool nb_handle_register_double(Handle handle, double value) {
  union HandleValue hval;
  hval.float64 = value;
  return register_handle(handle, TYPE_DOUBLE, hval);
}

bool nb_handle_register_voidp(Handle handle, void* value) {
  union HandleValue hval;
  hval.voidp = value;
  return register_handle(handle, TYPE_VOID_P, hval);
}

bool nb_handle_register_var(Handle handle, struct PP_Var value) {
  union HandleValue hval;
  hval.var = value;
  nb_var_addref(hval.var);

  switch (value.type) {
    case PP_VARTYPE_ARRAY_BUFFER:
    case PP_VARTYPE_ARRAY:
    case PP_VARTYPE_DICTIONARY:
    case PP_VARTYPE_STRING:
      return register_handle(handle, TYPE_VAR, hval);
    default:
      return FALSE;
  }
}

static bool get_handle(Handle handle, struct HandleObject* out_handle_object) {
  /* Binary search. */
  size_t lo_ix = 0;  /* Inclusive */
  size_t hi_ix = s_handle_map_size;  /* Exclusive */
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
#define TYPE_INT32_MIN (int32_t)(-0x80000000L)
#define TYPE_INT32_MAX (0x7fffffffL)
#define TYPE_INT64_MIN (int64_t)(-0x8000000000000000LL)
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

#define TYPE_SWITCH(to_type) \
  switch (hobj.type) { \
    TYPE_CASE(to_type, TYPE_INT8); \
    TYPE_CASE(to_type, TYPE_UINT8); \
    TYPE_CASE(to_type, TYPE_INT16); \
    TYPE_CASE(to_type, TYPE_UINT16); \
    TYPE_CASE(to_type, TYPE_INT32); \
    TYPE_CASE(to_type, TYPE_UINT32); \
    TYPE_CASE(to_type, TYPE_INT64); \
    TYPE_CASE(to_type, TYPE_UINT64); \
    TYPE_CASE(to_type, TYPE_FLOAT); \
    TYPE_CASE(to_type, TYPE_DOUBLE); \
    DEFAULT_TYPE_CASE(to_type); \
  }

#define TYPE_CASE(to_type, from_type) \
  case from_type: \
    CHECK_##to_type##_##from_type(); \
    *out_value = HOBJ_FIELD(from_type); \
    return TRUE  /* no semicolon */

#define CHECK_TYPE_INT8_TYPE_INT8()
#define CHECK_TYPE_INT8_TYPE_INT16() CHECK(TYPE_INT8, TYPE_INT16);
#define CHECK_TYPE_INT8_TYPE_INT32() CHECK(TYPE_INT8, TYPE_INT32);
#define CHECK_TYPE_INT8_TYPE_INT64() CHECK(TYPE_INT8, TYPE_INT64);
#define CHECK_TYPE_INT8_TYPE_UINT8()  CHECK_MAX(TYPE_INT8, TYPE_UINT8);
#define CHECK_TYPE_INT8_TYPE_UINT16() CHECK_MAX(TYPE_INT8, TYPE_UINT16);
#define CHECK_TYPE_INT8_TYPE_UINT32() CHECK_MAX(TYPE_INT8, TYPE_UINT32);
#define CHECK_TYPE_INT8_TYPE_UINT64() CHECK_MAX(TYPE_INT8, TYPE_UINT64);
#define CHECK_TYPE_INT8_TYPE_FLOAT() CHECK_FLOAT_TO_INT(TYPE_INT8, TYPE_FLOAT);
#define CHECK_TYPE_INT8_TYPE_DOUBLE() CHECK_DOUBLE_TO_INT(TYPE_INT8, TYPE_DOUBLE);

#define CHECK_TYPE_UINT8_TYPE_INT8()  CHECK_GT_ZERO(TYPE_UINT8, TYPE_INT8);
#define CHECK_TYPE_UINT8_TYPE_INT16() CHECK_MAX_GT_ZERO(TYPE_UINT8, TYPE_INT16);
#define CHECK_TYPE_UINT8_TYPE_INT32() CHECK_MAX_GT_ZERO(TYPE_UINT8, TYPE_INT32);
#define CHECK_TYPE_UINT8_TYPE_INT64() CHECK_MAX_GT_ZERO(TYPE_UINT8, TYPE_INT64);
#define CHECK_TYPE_UINT8_TYPE_UINT8()
#define CHECK_TYPE_UINT8_TYPE_UINT16() CHECK_MAX(TYPE_UINT8, TYPE_UINT16);
#define CHECK_TYPE_UINT8_TYPE_UINT32() CHECK_MAX(TYPE_UINT8, TYPE_UINT32);
#define CHECK_TYPE_UINT8_TYPE_UINT64() CHECK_MAX(TYPE_UINT8, TYPE_UINT64);
#define CHECK_TYPE_UINT8_TYPE_FLOAT() CHECK_FLOAT_TO_INT(TYPE_UINT8, TYPE_FLOAT);
#define CHECK_TYPE_UINT8_TYPE_DOUBLE() CHECK_DOUBLE_TO_INT(TYPE_UINT8, TYPE_DOUBLE);

#define CHECK_TYPE_INT16_TYPE_INT8()
#define CHECK_TYPE_INT16_TYPE_INT16()
#define CHECK_TYPE_INT16_TYPE_INT32() CHECK(TYPE_INT16, TYPE_INT32);
#define CHECK_TYPE_INT16_TYPE_INT64() CHECK(TYPE_INT16, TYPE_INT64);
#define CHECK_TYPE_INT16_TYPE_UINT8()
#define CHECK_TYPE_INT16_TYPE_UINT16() CHECK_MAX(TYPE_INT16, TYPE_UINT16);
#define CHECK_TYPE_INT16_TYPE_UINT32() CHECK_MAX(TYPE_INT16, TYPE_UINT32);
#define CHECK_TYPE_INT16_TYPE_UINT64() CHECK_MAX(TYPE_INT16, TYPE_UINT64);
#define CHECK_TYPE_INT16_TYPE_FLOAT() CHECK_FLOAT_TO_INT(TYPE_INT16, TYPE_FLOAT);
#define CHECK_TYPE_INT16_TYPE_DOUBLE() CHECK_DOUBLE_TO_INT(TYPE_INT16, TYPE_DOUBLE);

#define CHECK_TYPE_UINT16_TYPE_INT8()  CHECK_GT_ZERO(TYPE_UINT16, TYPE_INT8);
#define CHECK_TYPE_UINT16_TYPE_INT16() CHECK_GT_ZERO(TYPE_UINT16, TYPE_INT16);
#define CHECK_TYPE_UINT16_TYPE_INT32() CHECK_MAX_GT_ZERO(TYPE_UINT16, TYPE_INT32);
#define CHECK_TYPE_UINT16_TYPE_INT64() CHECK_MAX_GT_ZERO(TYPE_UINT16, TYPE_INT64);
#define CHECK_TYPE_UINT16_TYPE_UINT8()
#define CHECK_TYPE_UINT16_TYPE_UINT16()
#define CHECK_TYPE_UINT16_TYPE_UINT32() CHECK(TYPE_UINT16, TYPE_UINT32);
#define CHECK_TYPE_UINT16_TYPE_UINT64() CHECK(TYPE_UINT16, TYPE_UINT64);
#define CHECK_TYPE_UINT16_TYPE_FLOAT() CHECK_FLOAT_TO_INT(TYPE_UINT16, TYPE_FLOAT);
#define CHECK_TYPE_UINT16_TYPE_DOUBLE() CHECK_DOUBLE_TO_INT(TYPE_UINT16, TYPE_DOUBLE);

#define CHECK_TYPE_INT32_TYPE_INT8()
#define CHECK_TYPE_INT32_TYPE_INT16()
#define CHECK_TYPE_INT32_TYPE_INT32()
#define CHECK_TYPE_INT32_TYPE_INT64() CHECK(TYPE_INT32, TYPE_INT64);
#define CHECK_TYPE_INT32_TYPE_UINT8()
#define CHECK_TYPE_INT32_TYPE_UINT16()
#define CHECK_TYPE_INT32_TYPE_UINT32() CHECK_MAX(TYPE_INT32, TYPE_UINT32);
#define CHECK_TYPE_INT32_TYPE_UINT64() CHECK_MAX(TYPE_INT32, TYPE_UINT64);
#define CHECK_TYPE_INT32_TYPE_FLOAT() CHECK_FLOAT_TO_INT(TYPE_INT32, TYPE_FLOAT);
#define CHECK_TYPE_INT32_TYPE_DOUBLE() CHECK_DOUBLE_TO_INT(TYPE_INT32, TYPE_DOUBLE);

#define CHECK_TYPE_UINT32_TYPE_INT8()  CHECK_GT_ZERO(TYPE_UINT32, TYPE_INT8);
#define CHECK_TYPE_UINT32_TYPE_INT16() CHECK_GT_ZERO(TYPE_UINT32, TYPE_INT16);
#define CHECK_TYPE_UINT32_TYPE_INT32() CHECK_GT_ZERO(TYPE_UINT32, TYPE_INT32);
#define CHECK_TYPE_UINT32_TYPE_INT64() CHECK_MAX_GT_ZERO(TYPE_UINT32, TYPE_INT64);
#define CHECK_TYPE_UINT32_TYPE_UINT8()
#define CHECK_TYPE_UINT32_TYPE_UINT16()
#define CHECK_TYPE_UINT32_TYPE_UINT32()
#define CHECK_TYPE_UINT32_TYPE_UINT64() CHECK(TYPE_UINT32, TYPE_UINT64);
#define CHECK_TYPE_UINT32_TYPE_FLOAT() CHECK_FLOAT_TO_INT(TYPE_UINT32, TYPE_FLOAT);
#define CHECK_TYPE_UINT32_TYPE_DOUBLE() CHECK_DOUBLE_TO_INT(TYPE_UINT32, TYPE_DOUBLE);

#define CHECK_TYPE_INT64_TYPE_INT8()
#define CHECK_TYPE_INT64_TYPE_INT16()
#define CHECK_TYPE_INT64_TYPE_INT32()
#define CHECK_TYPE_INT64_TYPE_INT64()
#define CHECK_TYPE_INT64_TYPE_UINT8()
#define CHECK_TYPE_INT64_TYPE_UINT16()
#define CHECK_TYPE_INT64_TYPE_UINT32()
#define CHECK_TYPE_INT64_TYPE_UINT64() CHECK_MAX(TYPE_INT64, TYPE_UINT64);
#define CHECK_TYPE_INT64_TYPE_FLOAT() CHECK_FLOAT_TO_INT(TYPE_INT64, TYPE_FLOAT);
#define CHECK_TYPE_INT64_TYPE_DOUBLE() CHECK_DOUBLE_TO_INT(TYPE_INT64, TYPE_DOUBLE);

#define CHECK_TYPE_UINT64_TYPE_INT8()  CHECK_GT_ZERO(TYPE_UINT64, TYPE_INT8);
#define CHECK_TYPE_UINT64_TYPE_INT16() CHECK_GT_ZERO(TYPE_UINT64, TYPE_INT16);
#define CHECK_TYPE_UINT64_TYPE_INT32() CHECK_GT_ZERO(TYPE_UINT64, TYPE_INT32);
#define CHECK_TYPE_UINT64_TYPE_INT64() CHECK_GT_ZERO(TYPE_UINT64, TYPE_INT64);
#define CHECK_TYPE_UINT64_TYPE_UINT8()
#define CHECK_TYPE_UINT64_TYPE_UINT16()
#define CHECK_TYPE_UINT64_TYPE_UINT32()
#define CHECK_TYPE_UINT64_TYPE_UINT64()
#define CHECK_TYPE_UINT64_TYPE_FLOAT() CHECK_FLOAT_TO_INT(TYPE_UINT64, TYPE_FLOAT);
#define CHECK_TYPE_UINT64_TYPE_DOUBLE() CHECK_DOUBLE_TO_INT(TYPE_UINT64, TYPE_DOUBLE);

#define CHECK_TYPE_FLOAT_TYPE_INT8()
#define CHECK_TYPE_FLOAT_TYPE_INT16()
#define CHECK_TYPE_FLOAT_TYPE_INT32() CHECK_INT_TO_FLOAT(TYPE_FLOAT, TYPE_INT32);
#define CHECK_TYPE_FLOAT_TYPE_INT64() CHECK_INT_TO_FLOAT(TYPE_FLOAT, TYPE_INT64);
#define CHECK_TYPE_FLOAT_TYPE_UINT8()
#define CHECK_TYPE_FLOAT_TYPE_UINT16()
#define CHECK_TYPE_FLOAT_TYPE_UINT32() CHECK_MAX_INT_TO_FLOAT(TYPE_FLOAT, TYPE_UINT32);
#define CHECK_TYPE_FLOAT_TYPE_UINT64() CHECK_MAX_INT_TO_FLOAT(TYPE_FLOAT, TYPE_UINT64);
#define CHECK_TYPE_FLOAT_TYPE_FLOAT()
#define CHECK_TYPE_FLOAT_TYPE_DOUBLE()

#define CHECK_TYPE_DOUBLE_TYPE_INT8()
#define CHECK_TYPE_DOUBLE_TYPE_INT16()
#define CHECK_TYPE_DOUBLE_TYPE_INT32()
#define CHECK_TYPE_DOUBLE_TYPE_INT64() CHECK_INT_TO_DOUBLE(TYPE_FLOAT, TYPE_INT64);
#define CHECK_TYPE_DOUBLE_TYPE_UINT8()
#define CHECK_TYPE_DOUBLE_TYPE_UINT16()
#define CHECK_TYPE_DOUBLE_TYPE_UINT32()
#define CHECK_TYPE_DOUBLE_TYPE_UINT64() CHECK_MAX_INT_TO_DOUBLE(TYPE_FLOAT, TYPE_UINT64);
#define CHECK_TYPE_DOUBLE_TYPE_FLOAT()
#define CHECK_TYPE_DOUBLE_TYPE_DOUBLE()

#define DEFAULT_TYPE_CASE(to_type) \
  default: \
    VERROR("handle %d is of type %s. Expected %s.", handle, \
           nb_type_to_string(hobj.type), \
           nb_type_to_string(to_type)); \
    return FALSE  /* no semicolon */

#define CHECK(to_type, from_type) \
  if (HOBJ_FIELD(from_type) < to_type##_MIN || \
      HOBJ_FIELD(from_type) > to_type##_MAX) { \
    CHECK_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define CHECK_MAX(to_type, from_type) \
  if (HOBJ_FIELD(from_type) > to_type##_MAX) { \
    CHECK_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define CHECK_GT_ZERO(to_type, from_type) \
  if (HOBJ_FIELD(from_type) < 0) { \
    CHECK_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define CHECK_MAX_GT_ZERO(to_type, from_type) \
  if (HOBJ_FIELD(from_type) < 0 || \
      HOBJ_FIELD(from_type) > to_type##_MAX) { \
    CHECK_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define CHECK_SIGN(to_type, from_type) \
  CHECK(to_type, from_type)

#define CHECK_FLOAT_TO_INT(to_type, from_type) \
  if (HOBJ_FIELD(from_type) < to_type##_MIN || \
      HOBJ_FIELD(from_type) > to_type##_MAX || \
      HOBJ_FIELD(from_type) != rintf(HOBJ_FIELD(from_type))) { \
    CHECK_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define CHECK_FLOAT_TO_INT64(to_type, from_type) \
  if (HOBJ_FIELD(from_type) < to_type##_MIN_FLOAT || \
      HOBJ_FIELD(from_type) > to_type##_MAX_FLOAT || \
      HOBJ_FIELD(from_type) != rintf(HOBJ_FIELD(from_type))) { \
    CHECK_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define CHECK_DOUBLE_TO_INT(to_type, from_type) \
  if (HOBJ_FIELD(from_type) < to_type##_MIN || \
      HOBJ_FIELD(from_type) > to_type##_MAX || \
      HOBJ_FIELD(from_type) != rint(HOBJ_FIELD(from_type))) { \
    CHECK_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define CHECK_INT_TO_FLOAT(to_type, from_type) \
  if (HOBJ_FIELD(from_type) < TYPE_FLOAT_MIN_24 || \
      HOBJ_FIELD(from_type) > TYPE_FLOAT_MAX_24) { \
    CHECK_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define CHECK_MAX_INT_TO_FLOAT(to_type, from_type) \
  if (HOBJ_FIELD(from_type) > TYPE_FLOAT_MAX_24) { \
    CHECK_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define CHECK_INT_TO_DOUBLE(to_type, from_type) \
  if (HOBJ_FIELD(from_type) < TYPE_DOUBLE_MIN_53 || \
      HOBJ_FIELD(from_type) > TYPE_DOUBLE_MAX_53) { \
    CHECK_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define CHECK_MAX_INT_TO_DOUBLE(to_type, from_type) \
  if (HOBJ_FIELD(from_type) > TYPE_DOUBLE_MAX_53) { \
    CHECK_ERROR(to_type, from_type); \
    return FALSE; \
  }

#define CHECK_ERROR(to_type, from_type) \
  VERROR("handle %d(%s) with value " from_type##_FMT \
         " cannot be represented as %s.", handle, \
         nb_type_to_string(hobj.type), HOBJ_FIELD(from_type), \
         nb_type_to_string(to_type))

bool nb_handle_get_int8(Handle handle, int8_t* out_value) {
  struct HandleObject hobj;
  if (!get_handle(handle, &hobj)) {
    return FALSE;
  }

  TYPE_SWITCH(TYPE_INT8);
}

bool nb_handle_get_uint8(Handle handle, uint8_t* out_value) {
  struct HandleObject hobj;
  if (!get_handle(handle, &hobj)) {
    return FALSE;
  }

  TYPE_SWITCH(TYPE_UINT8);
}

bool nb_handle_get_int16(Handle handle, int16_t* out_value) {
  struct HandleObject hobj;
  if (!get_handle(handle, &hobj)) {
    return FALSE;
  }

  TYPE_SWITCH(TYPE_INT16);
}

bool nb_handle_get_uint16(Handle handle, uint16_t* out_value) {
  struct HandleObject hobj;
  if (!get_handle(handle, &hobj)) {
    return FALSE;
  }

  TYPE_SWITCH(TYPE_UINT16);
}

bool nb_handle_get_int32(Handle handle, int32_t* out_value) {
  struct HandleObject hobj;
  if (!get_handle(handle, &hobj)) {
    return FALSE;
  }

  TYPE_SWITCH(TYPE_INT32);
}

bool nb_handle_get_uint32(Handle handle, uint32_t* out_value) {
  struct HandleObject hobj;
  if (!get_handle(handle, &hobj)) {
    return FALSE;
  }

  TYPE_SWITCH(TYPE_UINT32);
}

bool nb_handle_get_int64(Handle handle, int64_t* out_value) {
  struct HandleObject hobj;
  if (!get_handle(handle, &hobj)) {
    return FALSE;
  }

  TYPE_SWITCH(TYPE_INT64);
}

bool nb_handle_get_uint64(Handle handle, uint64_t* out_value) {
  struct HandleObject hobj;
  if (!get_handle(handle, &hobj)) {
    return FALSE;
  }

  TYPE_SWITCH(TYPE_UINT64);
}

bool nb_handle_get_float(Handle handle, float* out_value) {
  struct HandleObject hobj;
  if (!get_handle(handle, &hobj)) {
    return FALSE;
  }

  TYPE_SWITCH(TYPE_FLOAT);
}

bool nb_handle_get_double(Handle handle, double* out_value) {
  struct HandleObject hobj;
  if (!get_handle(handle, &hobj)) {
    return FALSE;
  }

  TYPE_SWITCH(TYPE_DOUBLE);
}

static bool nb_hobj_string_value(struct HandleObject* hobj, char** out_value) {
  if (hobj->string_value == NULL) {
    uint32_t len;
    const char* str;
    if (!nb_var_string(hobj->value.var, &str, &len)) {
      return FALSE;
    }

    hobj->string_value = strndup(str, len);
  }

  *out_value = hobj->string_value;
  return TRUE;
}

bool nb_handle_get_voidp(Handle handle, void** out_value) {
  struct HandleObject hobj;
  if (!get_handle(handle, &hobj)) {
    return FALSE;
  }

  if (hobj.type == TYPE_VAR) {
    char* string_value;
    if (!nb_hobj_string_value(&hobj, &string_value)) {
      VERROR("unable to get string for handle %d", handle);
      return FALSE;
    }
    *out_value = string_value;
  } else if (hobj.type == TYPE_VOID_P) {
    *out_value = hobj.value.voidp;
  } else {
    VERROR("handle %d is of type %s. Expected %s.", handle,
           nb_type_to_string(hobj.type), nb_type_to_string(TYPE_VOID_P));
    return FALSE;
  }

  return TRUE;
}

bool nb_handle_get_charp(Handle handle, char** out_value) {
  struct HandleObject hobj;
  if (!get_handle(handle, &hobj)) {
    return FALSE;
  }

  if (hobj.type == TYPE_VAR) {
    char* string_value;
    if (!nb_hobj_string_value(&hobj, &string_value)) {
      VERROR("unable to get string for handle %d", handle);
      return FALSE;
    }
    *out_value = string_value;
  } else if (hobj.type == TYPE_VOID_P) {
    *out_value = (char*)hobj.value.voidp;
  } else {
    VERROR("handle %d is of type %s. Expected %s.", handle,
           nb_type_to_string(hobj.type), nb_type_to_string(TYPE_VOID_P));
    return FALSE;
  }

  return TRUE;
}

bool nb_handle_get_var(Handle handle, struct PP_Var* out_value) {
  struct HandleObject hobj;
  if (!get_handle(handle, &hobj)) {
    return FALSE;
  }

  if (hobj.type != TYPE_VAR) {
    VERROR("handle %d is of type %s. Expected %s.", handle,
           nb_type_to_string(hobj.type), nb_type_to_string(TYPE_VAR));
    return FALSE;
  }

  *out_value = hobj.value.var;
  return TRUE;
}

void nb_handle_destroy(Handle handle) {
  struct HandleMapPair* pair = NULL;


  /* Binary search. */
  size_t lo_ix = 0;  /* Inclusive */
  size_t hi_ix = s_handle_map_size;  /* Exclusive */
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

  if (pair->object.type == TYPE_VAR) {
    nb_var_release(pair->object.value.var);
  }

  free(pair->object.string_value);

  size_t remove_ix = mid_ix;
  if (remove_ix + 1 < s_handle_map_size) {
    memmove(
        &s_handle_map[remove_ix], &s_handle_map[remove_ix + 1],
        sizeof(struct HandleMapPair) * (s_handle_map_size - (remove_ix + 1)));
  }
  s_handle_map_size--;
}

void nb_handle_destroy_many(Handle* handles, uint32_t handles_count) {
  /* TODO(binji): optimize */
  uint32_t i;
  for (i = 0; i < handles_count; ++i) {
    Handle handle = handles[i];
    nb_handle_destroy(handle);
  }
}

bool nb_handle_convert_to_var(Handle handle, struct PP_Var* var) {
  struct HandleObject hobj;
  if (!get_handle(handle, &hobj)) {
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
    case TYPE_INT64:
      *var = nb_var_int64_create(hobj.value.int64);
      break;
    case TYPE_UINT64:
      *var = nb_var_int64_create((int64_t) hobj.value.uint64);
      break;
    case TYPE_FLOAT:
      var->type = PP_VARTYPE_DOUBLE;
      var->value.as_double = hobj.value.float32;
      break;
    case TYPE_DOUBLE:
      var->type = PP_VARTYPE_DOUBLE;
      var->value.as_double = hobj.value.float64;
      break;
    case TYPE_VAR:
      *var = hobj.value.var;
      nb_var_addref(*var);
      break;
    case TYPE_VOID_P:
      if (hobj.value.voidp) {
        var->type = PP_VARTYPE_INT32;
        var->value.as_int = handle;
      } else {
        var->type = PP_VARTYPE_NULL;
      }
      break;
    default:
      VERROR("Don't know how to convert handle %d with type %s to var", handle,
             nb_type_to_string(hobj.type));
      return FALSE;
  }

  return TRUE;
}
