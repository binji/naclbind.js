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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ppapi/c/pp_var.h>

#ifndef NB_ONE_FILE
#include "error.h"
#include "interfaces.h"
#include "var.h"
#endif

#define NB_HANDLE_MAP_INITIAL_CAPACITY 16

typedef union {
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
} NB_HandleValue;

typedef struct {
  NB_Type type;
  NB_HandleValue value;
  /* PP_Var strings are not guaranteed to be NULL-terminated, so if we want to
   * use it as a C string, we have to allocate space for a NULL and remember to
   * free it later.
   *
   * This field will be non-NULL when type == NB_TYPE_VAR and
   * nb_handle_get_charp() has been called. The memory will be free'd in
   * DestroyHandle.
   */
  char* string_value;
} NB_HandleObject;

typedef struct {
  NB_Handle handle;
  NB_HandleObject object;
} NB_HandleMapPair;

/* TODO(binji): use hashmap instead of sorted array. */
static NB_HandleMapPair* s_nb_handle_map = NULL;
static size_t s_nb_handle_map_size = 0;
static size_t s_nb_handle_map_capacity = 0;

static NB_Bool nb_resize_handle_map(size_t new_capacity) {
  assert(s_nb_handle_map_size <= new_capacity);
  s_nb_handle_map =
      realloc(s_nb_handle_map, sizeof(NB_HandleMapPair) * new_capacity);
  s_nb_handle_map_capacity = new_capacity;
  if (!s_nb_handle_map) {
    NB_ERROR("Out of memory");
    return NB_FALSE;
  }
  return NB_TRUE;
}

static NB_Bool nb_register_handle(NB_Handle handle,
                                  NB_Type type,
                                  NB_HandleValue value) {
  if (!s_nb_handle_map) {
    if (!nb_resize_handle_map(NB_HANDLE_MAP_INITIAL_CAPACITY)) {
      return NB_FALSE;
    }
  }

  if (s_nb_handle_map_size == s_nb_handle_map_capacity) {
    if (!nb_resize_handle_map(s_nb_handle_map_capacity * 2)) {
      return NB_FALSE;
    }
  }

  NB_HandleMapPair* pair = NULL;

  if (s_nb_handle_map_size == 0) {
    assert(s_nb_handle_map_capacity > 0);
    pair = &s_nb_handle_map[0];
  } else {
    /* Fast case, the new handle is larger than all other handles. */
    if (handle > s_nb_handle_map[s_nb_handle_map_size - 1].handle) {
      pair = &s_nb_handle_map[s_nb_handle_map_size];
    } else {
      /* Binary search to find the insertion point. */
      size_t lo_ix = 0;                    /* Inclusive */
      size_t hi_ix = s_nb_handle_map_size; /* Exclusive */
      while (lo_ix < hi_ix) {
        size_t mid_ix = (lo_ix + hi_ix) / 2;
        NB_Handle mid_handle = s_nb_handle_map[mid_ix].handle;
        if (handle > mid_handle) {
          lo_ix = mid_ix + 1;
        } else if (handle < mid_handle) {
          hi_ix = mid_ix;
        } else {
          NB_VERROR("handle %d is already registered.", handle);
          return NB_FALSE;
        }
      }

      /* Move everything after the insertion point down. */
      size_t insert_ix = lo_ix;
      if (insert_ix < s_nb_handle_map_size) {
        memmove(&s_nb_handle_map[insert_ix + 1],
                &s_nb_handle_map[insert_ix],
                sizeof(NB_HandleMapPair) * (s_nb_handle_map_size - insert_ix));
      }

      pair = &s_nb_handle_map[insert_ix];
    }
  }

  pair->handle = handle;
  pair->object.type = type;
  pair->object.value = value;
  pair->object.string_value = NULL;
  s_nb_handle_map_size++;
  return NB_TRUE;
}

int32_t nb_handle_count(void) {
  return s_nb_handle_map_size;
}

NB_Bool nb_handle_register_int8(NB_Handle handle, int8_t value) {
  NB_HandleValue hval;
  hval.int8 = value;
  return nb_register_handle(handle, NB_TYPE_INT8, hval);
}

NB_Bool nb_handle_register_uint8(NB_Handle handle, uint8_t value) {
  NB_HandleValue hval;
  hval.uint8 = value;
  return nb_register_handle(handle, NB_TYPE_UINT8, hval);
}

NB_Bool nb_handle_register_int16(NB_Handle handle, int16_t value) {
  NB_HandleValue hval;
  hval.int16 = value;
  return nb_register_handle(handle, NB_TYPE_INT16, hval);
}

NB_Bool nb_handle_register_uint16(NB_Handle handle, uint16_t value) {
  NB_HandleValue hval;
  hval.uint16 = value;
  return nb_register_handle(handle, NB_TYPE_UINT16, hval);
}

NB_Bool nb_handle_register_int32(NB_Handle handle, int32_t value) {
  NB_HandleValue hval;
  hval.int32 = value;
  return nb_register_handle(handle, NB_TYPE_INT32, hval);
}

NB_Bool nb_handle_register_uint32(NB_Handle handle, uint32_t value) {
  NB_HandleValue hval;
  hval.uint32 = value;
  return nb_register_handle(handle, NB_TYPE_UINT32, hval);
}

NB_Bool nb_handle_register_int64(NB_Handle handle, int64_t value) {
  NB_HandleValue hval;
  hval.int64 = value;
  return nb_register_handle(handle, NB_TYPE_INT64, hval);
}

NB_Bool nb_handle_register_uint64(NB_Handle handle, uint64_t value) {
  NB_HandleValue hval;
  hval.uint64 = value;
  return nb_register_handle(handle, NB_TYPE_UINT64, hval);
}

NB_Bool nb_handle_register_float(NB_Handle handle, float value) {
  NB_HandleValue hval;
  hval.float32 = value;
  return nb_register_handle(handle, NB_TYPE_FLOAT, hval);
}

NB_Bool nb_handle_register_double(NB_Handle handle, double value) {
  NB_HandleValue hval;
  hval.float64 = value;
  return nb_register_handle(handle, NB_TYPE_DOUBLE, hval);
}

NB_Bool nb_handle_register_voidp(NB_Handle handle, void* value) {
  NB_HandleValue hval;
  hval.voidp = value;
  return nb_register_handle(handle, NB_TYPE_VOID_P, hval);
}

NB_Bool nb_handle_register_var(NB_Handle handle, struct PP_Var value) {
  NB_HandleValue hval;
  hval.var = value;
  nb_var_addref(hval.var);

  switch (value.type) {
    case PP_VARTYPE_ARRAY_BUFFER:
    case PP_VARTYPE_ARRAY:
    case PP_VARTYPE_DICTIONARY:
    case PP_VARTYPE_STRING:
      return nb_register_handle(handle, NB_TYPE_VAR, hval);
    default:
      return NB_FALSE;
  }
}

static NB_Bool nb_get_handle(NB_Handle handle,
                             NB_HandleObject* out_handle_object) {
  /* Binary search. */
  size_t lo_ix = 0;                    /* Inclusive */
  size_t hi_ix = s_nb_handle_map_size; /* Exclusive */
  while (lo_ix < hi_ix) {
    size_t mid_ix = (lo_ix + hi_ix) / 2;
    NB_Handle mid_handle = s_nb_handle_map[mid_ix].handle;
    if (handle > mid_handle) {
      lo_ix = mid_ix + 1;
    } else if (handle < mid_handle) {
      hi_ix = mid_ix;
    } else {
      *out_handle_object = s_nb_handle_map[mid_ix].object;
      return NB_TRUE;
    }
  }

  return NB_FALSE;
}

#define NB_TYPE_INT8_MIN (-0x80)
#define NB_TYPE_INT8_MAX (0x7f)
#define NB_TYPE_INT16_MIN (-0x8000)
#define NB_TYPE_INT16_MAX (0x7fff)
#define NB_TYPE_INT32_MIN (int32_t)(-0x80000000L)
#define NB_TYPE_INT32_MAX (0x7fffffffL)
#define NB_TYPE_INT64_MIN (int64_t)(-0x8000000000000000LL)
#define NB_TYPE_INT64_MAX (0x7fffffffffffffffLL)
#define NB_TYPE_UINT8_MIN (0)
#define NB_TYPE_UINT8_MAX (0xff)
#define NB_TYPE_UINT16_MIN (0)
#define NB_TYPE_UINT16_MAX (0xffff)
#define NB_TYPE_UINT32_MIN (0)
#define NB_TYPE_UINT32_MAX (0xffffffffL)
#define NB_TYPE_UINT64_MIN (0)
#define NB_TYPE_UINT64_MAX (0xffffffffffffffffLL)
#define NB_TYPE_FLOAT_MIN_24 (-0xffffffL)
#define NB_TYPE_FLOAT_MAX_24 (0xffffffL)
#define NB_TYPE_DOUBLE_MIN_53 (-0x1fffffffffffffLL)
#define NB_TYPE_DOUBLE_MAX_53 (0x1fffffffffffffLL)

#define NB_TYPE_INT8_FMT "%d"
#define NB_TYPE_INT16_FMT "%d"
#define NB_TYPE_INT32_FMT "%d"
#define NB_TYPE_INT64_FMT "%lld"
#define NB_TYPE_UINT8_FMT "%u"
#define NB_TYPE_UINT16_FMT "%u"
#define NB_TYPE_UINT32_FMT "%u"
#define NB_TYPE_UINT64_FMT "%llu"
#define NB_TYPE_FLOAT_FMT "%g"
#define NB_TYPE_DOUBLE_FMT "%g"

#define NB_TYPE_INT8_FIELD int8
#define NB_TYPE_INT16_FIELD int16
#define NB_TYPE_INT32_FIELD int32
#define NB_TYPE_INT64_FIELD int64
#define NB_TYPE_UINT8_FIELD uint8
#define NB_TYPE_UINT16_FIELD uint16
#define NB_TYPE_UINT32_FIELD uint32
#define NB_TYPE_UINT64_FIELD uint64
#define NB_TYPE_FLOAT_FIELD float32
#define NB_TYPE_DOUBLE_FIELD float64

#define NB_HOBJ_FIELD(type) (hobj.value.type##_FIELD)

#define NB_TYPE_SWITCH(to_type, to)                 \
  switch (hobj.type) {                              \
    NB_TYPE_CASE(to_type, NB_TYPE_INT8, to, I8);    \
    NB_TYPE_CASE(to_type, NB_TYPE_UINT8, to, U8);   \
    NB_TYPE_CASE(to_type, NB_TYPE_INT16, to, I16);  \
    NB_TYPE_CASE(to_type, NB_TYPE_UINT16, to, U16); \
    NB_TYPE_CASE(to_type, NB_TYPE_INT32, to, I32);  \
    NB_TYPE_CASE(to_type, NB_TYPE_UINT32, to, U32); \
    NB_TYPE_CASE(to_type, NB_TYPE_INT64, to, I64);  \
    NB_TYPE_CASE(to_type, NB_TYPE_UINT64, to, U64); \
    NB_TYPE_CASE(to_type, NB_TYPE_FLOAT, to, FLT);  \
    NB_TYPE_CASE(to_type, NB_TYPE_DOUBLE, to, DBL); \
    NB_DEFAULT_TYPE_CASE(to_type);                  \
  }

#define NB_TYPE_CASE(to_type, from_type, to, from) \
  case from_type:                                  \
    NB_CHECK_##to##_##from();                      \
    *out_value = NB_HOBJ_FIELD(from_type);         \
    return NB_TRUE /* no semicolon */

#define NB_CHECK_I8_I8()
#define NB_CHECK_I8_I16() NB_CHECK(NB_TYPE_INT8, NB_TYPE_INT16);
#define NB_CHECK_I8_I32() NB_CHECK(NB_TYPE_INT8, NB_TYPE_INT32);
#define NB_CHECK_I8_I64() NB_CHECK(NB_TYPE_INT8, NB_TYPE_INT64);
#define NB_CHECK_I8_U8() NB_CHECK_MAX(NB_TYPE_INT8, NB_TYPE_UINT8);
#define NB_CHECK_I8_U16() NB_CHECK_MAX(NB_TYPE_INT8, NB_TYPE_UINT16);
#define NB_CHECK_I8_U32() NB_CHECK_MAX(NB_TYPE_INT8, NB_TYPE_UINT32);
#define NB_CHECK_I8_U64() NB_CHECK_MAX(NB_TYPE_INT8, NB_TYPE_UINT64);
#define NB_CHECK_I8_FLT() NB_CHECK_FLT_TO_INT(NB_TYPE_INT8, NB_TYPE_FLOAT);
#define NB_CHECK_I8_DBL() NB_CHECK_DBL_TO_INT(NB_TYPE_INT8, NB_TYPE_DOUBLE);

#define NB_CHECK_U8_I8() NB_CHECK_GT_ZERO(NB_TYPE_UINT8, NB_TYPE_INT8);
#define NB_CHECK_U8_I16() NB_CHECK_MAX_GT_ZERO(NB_TYPE_UINT8, NB_TYPE_INT16);
#define NB_CHECK_U8_I32() NB_CHECK_MAX_GT_ZERO(NB_TYPE_UINT8, NB_TYPE_INT32);
#define NB_CHECK_U8_I64() NB_CHECK_MAX_GT_ZERO(NB_TYPE_UINT8, NB_TYPE_INT64);
#define NB_CHECK_U8_U8()
#define NB_CHECK_U8_U16() NB_CHECK_MAX(NB_TYPE_UINT8, NB_TYPE_UINT16);
#define NB_CHECK_U8_U32() NB_CHECK_MAX(NB_TYPE_UINT8, NB_TYPE_UINT32);
#define NB_CHECK_U8_U64() NB_CHECK_MAX(NB_TYPE_UINT8, NB_TYPE_UINT64);
#define NB_CHECK_U8_FLT() NB_CHECK_FLT_TO_INT(NB_TYPE_UINT8, NB_TYPE_FLOAT);
#define NB_CHECK_U8_DBL() NB_CHECK_DBL_TO_INT(NB_TYPE_UINT8, NB_TYPE_DOUBLE);

#define NB_CHECK_I16_I8()
#define NB_CHECK_I16_I16()
#define NB_CHECK_I16_I32() NB_CHECK(NB_TYPE_INT16, NB_TYPE_INT32);
#define NB_CHECK_I16_I64() NB_CHECK(NB_TYPE_INT16, NB_TYPE_INT64);
#define NB_CHECK_I16_U8()
#define NB_CHECK_I16_U16() NB_CHECK_MAX(NB_TYPE_INT16, NB_TYPE_UINT16);
#define NB_CHECK_I16_U32() NB_CHECK_MAX(NB_TYPE_INT16, NB_TYPE_UINT32);
#define NB_CHECK_I16_U64() NB_CHECK_MAX(NB_TYPE_INT16, NB_TYPE_UINT64);
#define NB_CHECK_I16_FLT() NB_CHECK_FLT_TO_INT(NB_TYPE_INT16, NB_TYPE_FLOAT);
#define NB_CHECK_I16_DBL() NB_CHECK_DBL_TO_INT(NB_TYPE_INT16, NB_TYPE_DOUBLE);

#define NB_CHECK_U16_I8() NB_CHECK_GT_ZERO(NB_TYPE_UINT16, NB_TYPE_INT8);
#define NB_CHECK_U16_I16() NB_CHECK_GT_ZERO(NB_TYPE_UINT16, NB_TYPE_INT16);
#define NB_CHECK_U16_I32() NB_CHECK_MAX_GT_ZERO(NB_TYPE_UINT16, NB_TYPE_INT32);
#define NB_CHECK_U16_I64() NB_CHECK_MAX_GT_ZERO(NB_TYPE_UINT16, NB_TYPE_INT64);
#define NB_CHECK_U16_U8()
#define NB_CHECK_U16_U16()
#define NB_CHECK_U16_U32() NB_CHECK(NB_TYPE_UINT16, NB_TYPE_UINT32);
#define NB_CHECK_U16_U64() NB_CHECK(NB_TYPE_UINT16, NB_TYPE_UINT64);
#define NB_CHECK_U16_FLT() NB_CHECK_FLT_TO_INT(NB_TYPE_UINT16, NB_TYPE_FLOAT);
#define NB_CHECK_U16_DBL() NB_CHECK_DBL_TO_INT(NB_TYPE_UINT16, NB_TYPE_DOUBLE);

#define NB_CHECK_I32_I8()
#define NB_CHECK_I32_I16()
#define NB_CHECK_I32_I32()
#define NB_CHECK_I32_I64() NB_CHECK(NB_TYPE_INT32, NB_TYPE_INT64);
#define NB_CHECK_I32_U8()
#define NB_CHECK_I32_U16()
#define NB_CHECK_I32_U32() NB_CHECK_MAX(NB_TYPE_INT32, NB_TYPE_UINT32);
#define NB_CHECK_I32_U64() NB_CHECK_MAX(NB_TYPE_INT32, NB_TYPE_UINT64);
#define NB_CHECK_I32_FLT() NB_CHECK_FLT_TO_INT(NB_TYPE_INT32, NB_TYPE_FLOAT);
#define NB_CHECK_I32_DBL() NB_CHECK_DBL_TO_INT(NB_TYPE_INT32, NB_TYPE_DOUBLE);

#define NB_CHECK_U32_I8() NB_CHECK_GT_ZERO(NB_TYPE_UINT32, NB_TYPE_INT8);
#define NB_CHECK_U32_I16() NB_CHECK_GT_ZERO(NB_TYPE_UINT32, NB_TYPE_INT16);
#define NB_CHECK_U32_I32() NB_CHECK_GT_ZERO(NB_TYPE_UINT32, NB_TYPE_INT32);
#define NB_CHECK_U32_I64() NB_CHECK_MAX_GT_ZERO(NB_TYPE_UINT32, NB_TYPE_INT64);
#define NB_CHECK_U32_U8()
#define NB_CHECK_U32_U16()
#define NB_CHECK_U32_U32()
#define NB_CHECK_U32_U64() NB_CHECK(NB_TYPE_UINT32, NB_TYPE_UINT64);
#define NB_CHECK_U32_FLT() NB_CHECK_FLT_TO_INT(NB_TYPE_UINT32, NB_TYPE_FLOAT);
#define NB_CHECK_U32_DBL() NB_CHECK_DBL_TO_INT(NB_TYPE_UINT32, NB_TYPE_DOUBLE);

#define NB_CHECK_I64_I8()
#define NB_CHECK_I64_I16()
#define NB_CHECK_I64_I32()
#define NB_CHECK_I64_I64()
#define NB_CHECK_I64_U8()
#define NB_CHECK_I64_U16()
#define NB_CHECK_I64_U32()
#define NB_CHECK_I64_U64() NB_CHECK_MAX(NB_TYPE_INT64, NB_TYPE_UINT64);
#define NB_CHECK_I64_FLT() NB_CHECK_FLT_TO_INT(NB_TYPE_INT64, NB_TYPE_FLOAT);
#define NB_CHECK_I64_DBL() NB_CHECK_DBL_TO_INT(NB_TYPE_INT64, NB_TYPE_DOUBLE);

#define NB_CHECK_U64_I8() NB_CHECK_GT_ZERO(NB_TYPE_UINT64, NB_TYPE_INT8);
#define NB_CHECK_U64_I16() NB_CHECK_GT_ZERO(NB_TYPE_UINT64, NB_TYPE_INT16);
#define NB_CHECK_U64_I32() NB_CHECK_GT_ZERO(NB_TYPE_UINT64, NB_TYPE_INT32);
#define NB_CHECK_U64_I64() NB_CHECK_GT_ZERO(NB_TYPE_UINT64, NB_TYPE_INT64);
#define NB_CHECK_U64_U8()
#define NB_CHECK_U64_U16()
#define NB_CHECK_U64_U32()
#define NB_CHECK_U64_U64()
#define NB_CHECK_U64_FLT() NB_CHECK_FLT_TO_INT(NB_TYPE_UINT64, NB_TYPE_FLOAT);
#define NB_CHECK_U64_DBL() NB_CHECK_DBL_TO_INT(NB_TYPE_UINT64, NB_TYPE_DOUBLE);

#define NB_CHECK_FLT_I8()
#define NB_CHECK_FLT_I16()
#define NB_CHECK_FLT_I32() NB_CHECK_INT_TO_FLT(NB_TYPE_FLOAT, NB_TYPE_INT32);
#define NB_CHECK_FLT_I64() NB_CHECK_INT_TO_FLT(NB_TYPE_FLOAT, NB_TYPE_INT64);
#define NB_CHECK_FLT_U8()
#define NB_CHECK_FLT_U16()
#define NB_CHECK_FLT_U32() \
  NB_CHECK_MAX_INT_TO_FLT(NB_TYPE_FLOAT, NB_TYPE_UINT32);
#define NB_CHECK_FLT_U64() \
  NB_CHECK_MAX_INT_TO_FLT(NB_TYPE_FLOAT, NB_TYPE_UINT64);
#define NB_CHECK_FLT_FLT()
#define NB_CHECK_FLT_DBL()

#define NB_CHECK_DBL_I8()
#define NB_CHECK_DBL_I16()
#define NB_CHECK_DBL_I32()
#define NB_CHECK_DBL_I64() NB_CHECK_INT_TO_DBL(NB_TYPE_FLOAT, NB_TYPE_INT64);
#define NB_CHECK_DBL_U8()
#define NB_CHECK_DBL_U16()
#define NB_CHECK_DBL_U32()
#define NB_CHECK_DBL_U64() \
  NB_CHECK_MAX_INT_TO_DBL(NB_TYPE_FLOAT, NB_TYPE_UINT64);
#define NB_CHECK_DBL_FLT()
#define NB_CHECK_DBL_DBL()

#define NB_DEFAULT_TYPE_CASE(to_type)                  \
  default:                                             \
    NB_VERROR("handle %d is of type %s. Expected %s.", \
              handle,                                  \
              nb_type_to_string(hobj.type),            \
              nb_type_to_string(to_type));             \
    return NB_FALSE /* no semicolon */

#define NB_CHECK(to_type, from_type)              \
  if (NB_HOBJ_FIELD(from_type) < to_type##_MIN || \
      NB_HOBJ_FIELD(from_type) > to_type##_MAX) { \
    NB_CHECK_ERROR(to_type, from_type);           \
    return NB_FALSE;                              \
  }

#define NB_CHECK_MAX(to_type, from_type)          \
  if (NB_HOBJ_FIELD(from_type) > to_type##_MAX) { \
    NB_CHECK_ERROR(to_type, from_type);           \
    return NB_FALSE;                              \
  }

#define NB_CHECK_GT_ZERO(to_type, from_type) \
  if (NB_HOBJ_FIELD(from_type) < 0) {        \
    NB_CHECK_ERROR(to_type, from_type);      \
    return NB_FALSE;                         \
  }

#define NB_CHECK_MAX_GT_ZERO(to_type, from_type)  \
  if (NB_HOBJ_FIELD(from_type) < 0 ||             \
      NB_HOBJ_FIELD(from_type) > to_type##_MAX) { \
    NB_CHECK_ERROR(to_type, from_type);           \
    return NB_FALSE;                              \
  }

#define NB_CHECK_SIGN(to_type, from_type) NB_CHECK(to_type, from_type)

#define NB_CHECK_FLT_TO_INT(to_type, from_type)                      \
  if (NB_HOBJ_FIELD(from_type) < to_type##_MIN ||                    \
      NB_HOBJ_FIELD(from_type) > to_type##_MAX ||                    \
      NB_HOBJ_FIELD(from_type) != rintf(NB_HOBJ_FIELD(from_type))) { \
    NB_CHECK_ERROR(to_type, from_type);                              \
    return NB_FALSE;                                                 \
  }

#define NB_CHECK_FLOAT_TO_INT64(to_type, from_type)                  \
  if (NB_HOBJ_FIELD(from_type) < to_type##_MIN_FLOAT ||              \
      NB_HOBJ_FIELD(from_type) > to_type##_MAX_FLOAT ||              \
      NB_HOBJ_FIELD(from_type) != rintf(NB_HOBJ_FIELD(from_type))) { \
    NB_CHECK_ERROR(to_type, from_type);                              \
    return NB_FALSE;                                                 \
  }

#define NB_CHECK_DBL_TO_INT(to_type, from_type)                     \
  if (NB_HOBJ_FIELD(from_type) < to_type##_MIN ||                   \
      NB_HOBJ_FIELD(from_type) > to_type##_MAX ||                   \
      NB_HOBJ_FIELD(from_type) != rint(NB_HOBJ_FIELD(from_type))) { \
    NB_CHECK_ERROR(to_type, from_type);                             \
    return NB_FALSE;                                                \
  }

#define NB_CHECK_INT_TO_FLT(to_type, from_type)          \
  if (NB_HOBJ_FIELD(from_type) < NB_TYPE_FLOAT_MIN_24 || \
      NB_HOBJ_FIELD(from_type) > NB_TYPE_FLOAT_MAX_24) { \
    NB_CHECK_ERROR(to_type, from_type);                  \
    return NB_FALSE;                                     \
  }

#define NB_CHECK_MAX_INT_TO_FLT(to_type, from_type)      \
  if (NB_HOBJ_FIELD(from_type) > NB_TYPE_FLOAT_MAX_24) { \
    NB_CHECK_ERROR(to_type, from_type);                  \
    return NB_FALSE;                                     \
  }

#define NB_CHECK_INT_TO_DBL(to_type, from_type)           \
  if (NB_HOBJ_FIELD(from_type) < NB_TYPE_DOUBLE_MIN_53 || \
      NB_HOBJ_FIELD(from_type) > NB_TYPE_DOUBLE_MAX_53) { \
    NB_CHECK_ERROR(to_type, from_type);                   \
    return NB_FALSE;                                      \
  }

#define NB_CHECK_MAX_INT_TO_DBL(to_type, from_type)       \
  if (NB_HOBJ_FIELD(from_type) > NB_TYPE_DOUBLE_MAX_53) { \
    NB_CHECK_ERROR(to_type, from_type);                   \
    return NB_FALSE;                                      \
  }

#define NB_CHECK_ERROR(to_type, from_type)              \
  NB_VERROR("handle %d(%s) with value " from_type##_FMT \
            " cannot be represented as %s.",            \
            handle,                                     \
            nb_type_to_string(hobj.type),               \
            NB_HOBJ_FIELD(from_type),                   \
            nb_type_to_string(to_type))

NB_Bool nb_handle_get_int8(NB_Handle handle, int8_t* out_value) {
  NB_HandleObject hobj;
  if (!nb_get_handle(handle, &hobj)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_INT8, I8);
}

NB_Bool nb_handle_get_uint8(NB_Handle handle, uint8_t* out_value) {
  NB_HandleObject hobj;
  if (!nb_get_handle(handle, &hobj)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_UINT8, U8);
}

NB_Bool nb_handle_get_int16(NB_Handle handle, int16_t* out_value) {
  NB_HandleObject hobj;
  if (!nb_get_handle(handle, &hobj)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_INT16, I16);
}

NB_Bool nb_handle_get_uint16(NB_Handle handle, uint16_t* out_value) {
  NB_HandleObject hobj;
  if (!nb_get_handle(handle, &hobj)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_UINT16, U16);
}

NB_Bool nb_handle_get_int32(NB_Handle handle, int32_t* out_value) {
  NB_HandleObject hobj;
  if (!nb_get_handle(handle, &hobj)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_INT32, I32);
}

NB_Bool nb_handle_get_uint32(NB_Handle handle, uint32_t* out_value) {
  NB_HandleObject hobj;
  if (!nb_get_handle(handle, &hobj)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_UINT32, U32);
}

NB_Bool nb_handle_get_int64(NB_Handle handle, int64_t* out_value) {
  NB_HandleObject hobj;
  if (!nb_get_handle(handle, &hobj)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_INT64, I64);
}

NB_Bool nb_handle_get_uint64(NB_Handle handle, uint64_t* out_value) {
  NB_HandleObject hobj;
  if (!nb_get_handle(handle, &hobj)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_UINT64, U64);
}

NB_Bool nb_handle_get_float(NB_Handle handle, float* out_value) {
  NB_HandleObject hobj;
  if (!nb_get_handle(handle, &hobj)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_FLOAT, FLT);
}

NB_Bool nb_handle_get_double(NB_Handle handle, double* out_value) {
  NB_HandleObject hobj;
  if (!nb_get_handle(handle, &hobj)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_DOUBLE, DBL);
}

#undef NB_TYPE_INT8_MIN
#undef NB_TYPE_INT8_MAX
#undef NB_TYPE_INT16_MIN
#undef NB_TYPE_INT16_MAX
#undef NB_TYPE_INT32_MIN
#undef NB_TYPE_INT32_MAX
#undef NB_TYPE_INT64_MIN
#undef NB_TYPE_INT64_MAX
#undef NB_TYPE_UINT8_MIN
#undef NB_TYPE_UINT8_MAX
#undef NB_TYPE_UINT16_MIN
#undef NB_TYPE_UINT16_MAX
#undef NB_TYPE_UINT32_MIN
#undef NB_TYPE_UINT32_MAX
#undef NB_TYPE_UINT64_MIN
#undef NB_TYPE_UINT64_MAX
#undef NB_TYPE_FLOAT_MIN_24
#undef NB_TYPE_FLOAT_MAX_24
#undef NB_TYPE_DOUBLE_MIN_53
#undef NB_TYPE_DOUBLE_MAX_53
#undef NB_TYPE_INT8_FMT
#undef NB_TYPE_INT16_FMT
#undef NB_TYPE_INT32_FMT
#undef NB_TYPE_INT64_FMT
#undef NB_TYPE_UINT8_FMT
#undef NB_TYPE_UINT16_FMT
#undef NB_TYPE_UINT32_FMT
#undef NB_TYPE_UINT64_FMT
#undef NB_TYPE_FLOAT_FMT
#undef NB_TYPE_DOUBLE_FMT
#undef NB_TYPE_INT8_FIELD
#undef NB_TYPE_INT16_FIELD
#undef NB_TYPE_INT32_FIELD
#undef NB_TYPE_INT64_FIELD
#undef NB_TYPE_UINT8_FIELD
#undef NB_TYPE_UINT16_FIELD
#undef NB_TYPE_UINT32_FIELD
#undef NB_TYPE_UINT64_FIELD
#undef NB_TYPE_FLOAT_FIELD
#undef NB_TYPE_DOUBLE_FIELD
#undef NB_HOBJ_FIELD
#undef NB_TYPE_SWITCH
#undef NB_TYPE_CASE
#undef NB_CHECK_TYPE_INT8_TYPE_INT8
#undef NB_CHECK_TYPE_INT8_TYPE_INT16
#undef NB_CHECK_TYPE_INT8_TYPE_INT32
#undef NB_CHECK_TYPE_INT8_TYPE_INT64
#undef NB_CHECK_TYPE_INT8_TYPE_UINT8
#undef NB_CHECK_TYPE_INT8_TYPE_UINT16
#undef NB_CHECK_TYPE_INT8_TYPE_UINT32
#undef NB_CHECK_TYPE_INT8_TYPE_UINT64
#undef NB_CHECK_TYPE_INT8_TYPE_FLOAT
#undef NB_CHECK_TYPE_INT8_TYPE_DOUBLE
#undef NB_CHECK_TYPE_UINT8_TYPE_INT8
#undef NB_CHECK_TYPE_UINT8_TYPE_INT16
#undef NB_CHECK_TYPE_UINT8_TYPE_INT32
#undef NB_CHECK_TYPE_UINT8_TYPE_INT64
#undef NB_CHECK_TYPE_UINT8_TYPE_UINT8
#undef NB_CHECK_TYPE_UINT8_TYPE_UINT16
#undef NB_CHECK_TYPE_UINT8_TYPE_UINT32
#undef NB_CHECK_TYPE_UINT8_TYPE_UINT64
#undef NB_CHECK_TYPE_UINT8_TYPE_FLOAT
#undef NB_CHECK_TYPE_UINT8_TYPE_DOUBLE
#undef NB_CHECK_TYPE_INT16_TYPE_INT8
#undef NB_CHECK_TYPE_INT16_TYPE_INT16
#undef NB_CHECK_TYPE_INT16_TYPE_INT32
#undef NB_CHECK_TYPE_INT16_TYPE_INT64
#undef NB_CHECK_TYPE_INT16_TYPE_UINT8
#undef NB_CHECK_TYPE_INT16_TYPE_UINT16
#undef NB_CHECK_TYPE_INT16_TYPE_UINT32
#undef NB_CHECK_TYPE_INT16_TYPE_UINT64
#undef NB_CHECK_TYPE_INT16_TYPE_FLOAT
#undef NB_CHECK_TYPE_INT16_TYPE_DOUBLE
#undef NB_CHECK_TYPE_UINT16_TYPE_INT8
#undef NB_CHECK_TYPE_UINT16_TYPE_INT16
#undef NB_CHECK_TYPE_UINT16_TYPE_INT32
#undef NB_CHECK_TYPE_UINT16_TYPE_INT64
#undef NB_CHECK_TYPE_UINT16_TYPE_UINT8
#undef NB_CHECK_TYPE_UINT16_TYPE_UINT16
#undef NB_CHECK_TYPE_UINT16_TYPE_UINT32
#undef NB_CHECK_TYPE_UINT16_TYPE_UINT64
#undef NB_CHECK_TYPE_UINT16_TYPE_FLOAT
#undef NB_CHECK_TYPE_UINT16_TYPE_DOUBLE
#undef NB_CHECK_TYPE_INT32_TYPE_INT8
#undef NB_CHECK_TYPE_INT32_TYPE_INT16
#undef NB_CHECK_TYPE_INT32_TYPE_INT32
#undef NB_CHECK_TYPE_INT32_TYPE_INT64
#undef NB_CHECK_TYPE_INT32_TYPE_UINT8
#undef NB_CHECK_TYPE_INT32_TYPE_UINT16
#undef NB_CHECK_TYPE_INT32_TYPE_UINT32
#undef NB_CHECK_TYPE_INT32_TYPE_UINT64
#undef NB_CHECK_TYPE_INT32_TYPE_FLOAT
#undef NB_CHECK_TYPE_INT32_TYPE_DOUBLE
#undef NB_CHECK_TYPE_UINT32_TYPE_INT8
#undef NB_CHECK_TYPE_UINT32_TYPE_INT16
#undef NB_CHECK_TYPE_UINT32_TYPE_INT32
#undef NB_CHECK_TYPE_UINT32_TYPE_INT64
#undef NB_CHECK_TYPE_UINT32_TYPE_UINT8
#undef NB_CHECK_TYPE_UINT32_TYPE_UINT16
#undef NB_CHECK_TYPE_UINT32_TYPE_UINT32
#undef NB_CHECK_TYPE_UINT32_TYPE_UINT64
#undef NB_CHECK_TYPE_UINT32_TYPE_FLOAT
#undef NB_CHECK_TYPE_UINT32_TYPE_DOUBLE
#undef NB_CHECK_TYPE_INT64_TYPE_INT8
#undef NB_CHECK_TYPE_INT64_TYPE_INT16
#undef NB_CHECK_TYPE_INT64_TYPE_INT32
#undef NB_CHECK_TYPE_INT64_TYPE_INT64
#undef NB_CHECK_TYPE_INT64_TYPE_UINT8
#undef NB_CHECK_TYPE_INT64_TYPE_UINT16
#undef NB_CHECK_TYPE_INT64_TYPE_UINT32
#undef NB_CHECK_TYPE_INT64_TYPE_UINT64
#undef NB_CHECK_TYPE_INT64_TYPE_FLOAT
#undef NB_CHECK_TYPE_INT64_TYPE_DOUBLE
#undef NB_CHECK_TYPE_UINT64_TYPE_INT8
#undef NB_CHECK_TYPE_UINT64_TYPE_INT16
#undef NB_CHECK_TYPE_UINT64_TYPE_INT32
#undef NB_CHECK_TYPE_UINT64_TYPE_INT64
#undef NB_CHECK_TYPE_UINT64_TYPE_UINT8
#undef NB_CHECK_TYPE_UINT64_TYPE_UINT16
#undef NB_CHECK_TYPE_UINT64_TYPE_UINT32
#undef NB_CHECK_TYPE_UINT64_TYPE_UINT64
#undef NB_CHECK_TYPE_UINT64_TYPE_FLOAT
#undef NB_CHECK_TYPE_UINT64_TYPE_DOUBLE
#undef NB_CHECK_TYPE_FLOAT_TYPE_INT8
#undef NB_CHECK_TYPE_FLOAT_TYPE_INT16
#undef NB_CHECK_TYPE_FLOAT_TYPE_INT32
#undef NB_CHECK_TYPE_FLOAT_TYPE_INT64
#undef NB_CHECK_TYPE_FLOAT_TYPE_UINT8
#undef NB_CHECK_TYPE_FLOAT_TYPE_UINT16
#undef NB_CHECK_TYPE_FLOAT_TYPE_UINT32
#undef NB_CHECK_TYPE_FLOAT_TYPE_UINT64
#undef NB_CHECK_TYPE_FLOAT_TYPE_FLOAT
#undef NB_CHECK_TYPE_FLOAT_TYPE_DOUBLE
#undef NB_CHECK_TYPE_DOUBLE_TYPE_INT8
#undef NB_CHECK_TYPE_DOUBLE_TYPE_INT16
#undef NB_CHECK_TYPE_DOUBLE_TYPE_INT32
#undef NB_CHECK_TYPE_DOUBLE_TYPE_INT64
#undef NB_CHECK_TYPE_DOUBLE_TYPE_UINT8
#undef NB_CHECK_TYPE_DOUBLE_TYPE_UINT16
#undef NB_CHECK_TYPE_DOUBLE_TYPE_UINT32
#undef NB_CHECK_TYPE_DOUBLE_TYPE_UINT64
#undef NB_CHECK_TYPE_DOUBLE_TYPE_FLOAT
#undef NB_CHECK_TYPE_DOUBLE_TYPE_DOUBLE
#undef NB_DEFAULT_TYPE_CASE
#undef NB_CHECK
#undef NB_CHECK_MAX
#undef NB_CHECK_GT_ZERO
#undef NB_CHECK_MAX_GT_ZERO
#undef NB_CHECK_SIGN
#undef NB_CHECK_FLT_TO_INT
#undef NB_CHECK_FLOAT_TO_INT64
#undef NB_CHECK_DBL_TO_INT
#undef NB_CHECK_INT_TO_FLT
#undef NB_CHECK_MAX_INT_TO_FLT
#undef NB_CHECK_INT_TO_DBL
#undef NB_CHECK_MAX_INT_TO_DBL
#undef NB_CHECK_ERROR

static NB_Bool nb_hobj_string_value(NB_HandleObject* hobj, char** out_value) {
  if (hobj->string_value == NULL) {
    uint32_t len;
    const char* str;
    if (!nb_var_string(hobj->value.var, &str, &len)) {
      return NB_FALSE;
    }

    hobj->string_value = strndup(str, len);
  }

  *out_value = hobj->string_value;
  return NB_TRUE;
}

NB_Bool nb_handle_get_voidp(NB_Handle handle, void** out_value) {
  NB_HandleObject hobj;
  if (!nb_get_handle(handle, &hobj)) {
    return NB_FALSE;
  }

  if (hobj.type == NB_TYPE_VAR) {
    char* string_value;
    if (!nb_hobj_string_value(&hobj, &string_value)) {
      NB_VERROR("unable to get string for handle %d", handle);
      return NB_FALSE;
    }
    *out_value = string_value;
  } else if (hobj.type == NB_TYPE_VOID_P) {
    *out_value = hobj.value.voidp;
  } else {
    NB_VERROR("handle %d is of type %s. Expected %s.",
              handle,
              nb_type_to_string(hobj.type),
              nb_type_to_string(NB_TYPE_VOID_P));
    return NB_FALSE;
  }

  return NB_TRUE;
}

NB_Bool nb_handle_get_charp(NB_Handle handle, char** out_value) {
  NB_HandleObject hobj;
  if (!nb_get_handle(handle, &hobj)) {
    return NB_FALSE;
  }

  if (hobj.type == NB_TYPE_VAR) {
    char* string_value;
    if (!nb_hobj_string_value(&hobj, &string_value)) {
      NB_VERROR("unable to get string for handle %d", handle);
      return NB_FALSE;
    }
    *out_value = string_value;
  } else if (hobj.type == NB_TYPE_VOID_P) {
    *out_value = (char*)hobj.value.voidp;
  } else {
    NB_VERROR("handle %d is of type %s. Expected %s.",
              handle,
              nb_type_to_string(hobj.type),
              nb_type_to_string(NB_TYPE_VOID_P));
    return NB_FALSE;
  }

  return NB_TRUE;
}

NB_Bool nb_handle_get_var(NB_Handle handle, struct PP_Var* out_value) {
  NB_HandleObject hobj;
  if (!nb_get_handle(handle, &hobj)) {
    return NB_FALSE;
  }

  if (hobj.type != NB_TYPE_VAR) {
    NB_VERROR("handle %d is of type %s. Expected %s.",
              handle,
              nb_type_to_string(hobj.type),
              nb_type_to_string(NB_TYPE_VAR));
    return NB_FALSE;
  }

  *out_value = hobj.value.var;
  return NB_TRUE;
}

NB_Bool nb_handle_get_default(NB_Handle handle,
                              NB_VarArgInt** iargs,
                              NB_VarArgInt* max_iargs,
                              NB_VarArgDbl** dargs,
                              NB_VarArgDbl* max_dargs) {
  NB_HandleObject hobj;
  if (!nb_get_handle(handle, &hobj)) {
    return NB_FALSE;
  }

#define NB_CHECK(args, max_args, needed, type) \
  if (args + needed - 1 >= max_args) {         \
    NB_ERROR("Too many " type "args passed."); \
    return NB_FALSE;                           \
  }

#ifdef __x86_64__
#define NB_PUSH_INT(field)                                  \
  NB_CHECK(*iargs, max_iargs, 1, "non-float ")*(*iargs)++ = \
      (uint64_t)(int64_t)hobj.value.field

#define NB_PUSH_UINT(field)                                 \
  NB_CHECK(*iargs, max_iargs, 1, "non-float ")*(*iargs)++ = \
      (uint64_t)hobj.value.field

#define NB_PUSH_INT64(field) NB_PUSH_INT(field)
#define NB_PUSH_UINT64(field) NB_PUSH_UINT(field)

#define NB_PUSH_DOUBLE(field) \
  NB_CHECK(*dargs, max_dargs, 1, "float ")*(*dargs)++ = (double)hobj.value.field

#define NB_PUSH_VOIDP(field)                                \
  NB_CHECK(*iargs, max_iargs, 1, "non-float ")*(*iargs)++ = \
      (uint64_t)(uint32_t)hobj.value.field

#else

  union {
    struct {
      uint32_t lo;
      uint32_t hi;
    };
    int32_t int32;
    uint32_t uint32;
    int64_t int64;
    uint64_t uint64;
    double float64;
  } x;

#define NB_PUSH_INT(field)                                       \
  NB_CHECK(*iargs, max_iargs, 1, "") x.int32 = hobj.value.field; \
  *(*iargs)++ = x.lo

#define NB_PUSH_UINT(field)                                       \
  NB_CHECK(*iargs, max_iargs, 1, "") x.uint32 = hobj.value.field; \
  *(*iargs)++ = x.lo

#define NB_PUSH_INT64(field)                                     \
  NB_CHECK(*iargs, max_iargs, 2, "") x.int64 = hobj.value.field; \
  *(*iargs)++ = x.lo;                                            \
  *(*iargs)++ = x.hi

#define NB_PUSH_UINT64(field)                                     \
  NB_CHECK(*iargs, max_iargs, 2, "") x.uint64 = hobj.value.field; \
  *(*iargs)++ = x.lo;                                             \
  *(*iargs)++ = x.hi

#define NB_PUSH_DOUBLE(field)                                      \
  NB_CHECK(*iargs, max_iargs, 2, "") x.float64 = hobj.value.field; \
  *(*iargs)++ = x.lo;                                              \
  *(*iargs)++ = x.hi

#define NB_PUSH_VOIDP(field) \
  NB_CHECK(*iargs, max_iargs, 1, "")*(*iargs)++ = (uint32_t)hobj.value.field
#endif

  switch (hobj.type) {
    case NB_TYPE_INT8:
      NB_PUSH_INT(int8);
      return NB_TRUE;
    case NB_TYPE_UINT8:
      NB_PUSH_UINT(uint8);
      return NB_TRUE;
    case NB_TYPE_INT16:
      NB_PUSH_INT(int16);
      return NB_TRUE;
    case NB_TYPE_UINT16:
      NB_PUSH_INT(uint16);
      return NB_TRUE;
    case NB_TYPE_INT32:
      NB_PUSH_INT(int32);
      return NB_TRUE;
    case NB_TYPE_UINT32:
      NB_PUSH_UINT(uint32);
      return NB_TRUE;
    case NB_TYPE_INT64:
      NB_PUSH_INT64(int64);
      return NB_TRUE;
    case NB_TYPE_UINT64:
      NB_PUSH_UINT64(uint64);
      return NB_TRUE;
    case NB_TYPE_FLOAT:
      NB_PUSH_DOUBLE(float32);
      return NB_TRUE;
    case NB_TYPE_DOUBLE:
      NB_PUSH_DOUBLE(float64);
      return NB_TRUE;
    case NB_TYPE_VOID_P:
      NB_PUSH_VOIDP(voidp);
      return NB_TRUE;

    default:
      NB_VERROR("Invalid type %s, can\'t make default promotion.",
                nb_type_to_string(hobj.type));
      return NB_FALSE;
  }
}

#undef NB_CHECK
#undef NB_PUSH_INT
#undef NB_PUSH_UINT
#undef NB_PUSH_INT64
#undef NB_PUSH_UINT64
#undef NB_PUSH_DOUBLE
#undef NB_PUSH_VOIDP

void nb_handle_destroy(NB_Handle handle) {
  NB_HandleMapPair* pair = NULL;

  /* Binary search. */
  size_t lo_ix = 0;                    /* Inclusive */
  size_t hi_ix = s_nb_handle_map_size; /* Exclusive */
  size_t mid_ix;

  while (lo_ix < hi_ix) {
    mid_ix = (lo_ix + hi_ix) / 2;
    NB_Handle mid_handle = s_nb_handle_map[mid_ix].handle;
    if (handle > mid_handle) {
      lo_ix = mid_ix + 1;
    } else if (handle < mid_handle) {
      hi_ix = mid_ix;
    } else {
      pair = &s_nb_handle_map[mid_ix];
      break;
    }
  }

  if (pair == NULL) {
    NB_VERROR("Destroying handle %d, but it doesn't exist.", handle);
    return;
  }

  if (pair->object.type == NB_TYPE_VAR) {
    nb_var_release(pair->object.value.var);
  }

  free(pair->object.string_value);

  size_t remove_ix = mid_ix;
  if (remove_ix + 1 < s_nb_handle_map_size) {
    memmove(
        &s_nb_handle_map[remove_ix],
        &s_nb_handle_map[remove_ix + 1],
        sizeof(NB_HandleMapPair) * (s_nb_handle_map_size - (remove_ix + 1)));
  }
  s_nb_handle_map_size--;
}

void nb_handle_destroy_many(NB_Handle* handles, uint32_t handles_count) {
  /* TODO(binji): optimize */
  uint32_t i;
  for (i = 0; i < handles_count; ++i) {
    NB_Handle handle = handles[i];
    nb_handle_destroy(handle);
  }
}

NB_Bool nb_handle_convert_to_var(NB_Handle handle, struct PP_Var* var) {
  NB_HandleObject hobj;
  if (!nb_get_handle(handle, &hobj)) {
    return NB_FALSE;
  }

  switch (hobj.type) {
    case NB_TYPE_INT8:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hobj.value.int8;
      break;
    case NB_TYPE_UINT8:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hobj.value.uint8;
      break;
    case NB_TYPE_INT16:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hobj.value.int16;
      break;
    case NB_TYPE_UINT16:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hobj.value.uint16;
      break;
    case NB_TYPE_INT32:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hobj.value.int32;
      break;
    case NB_TYPE_UINT32:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hobj.value.uint32;
      break;
    case NB_TYPE_INT64:
      *var = nb_var_int64_create(hobj.value.int64);
      break;
    case NB_TYPE_UINT64:
      *var = nb_var_int64_create((int64_t)hobj.value.uint64);
      break;
    case NB_TYPE_FLOAT:
      var->type = PP_VARTYPE_DOUBLE;
      var->value.as_double = hobj.value.float32;
      break;
    case NB_TYPE_DOUBLE:
      var->type = PP_VARTYPE_DOUBLE;
      var->value.as_double = hobj.value.float64;
      break;
    case NB_TYPE_VAR:
      *var = hobj.value.var;
      nb_var_addref(*var);
      break;
    case NB_TYPE_VOID_P:
      if (hobj.value.voidp) {
        var->type = PP_VARTYPE_INT32;
        var->value.as_int = handle;
      } else {
        var->type = PP_VARTYPE_NULL;
      }
      break;
    default:
      NB_VERROR("Don't know how to convert handle %d with type %s to var",
                handle,
                nb_type_to_string(hobj.type));
      return NB_FALSE;
  }

  return NB_TRUE;
}
