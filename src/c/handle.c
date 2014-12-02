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
#define NB_INVALID_HANDLE 0

typedef size_t NB_HashIndex;

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
  void (*funcp)(void);
  struct PP_Var var;
} NB_HandleValue;

typedef struct NB_HandleMapEntry {
  NB_Handle handle;
  NB_Type type;
  NB_HandleValue value;
  struct NB_HandleMapEntry* next;
  union {
    /* PP_Var strings are not guaranteed to be NULL-terminated, so if we want
     * to use it as a C string, we have to allocate space for a NULL and
     * remember to free it later.
     *
     * This field will be non-NULL when type == NB_TYPE_VAR and
     * nb_handle_get_charp() has been called. The memory will be free'd in
     * DestroyHandle.
     */
    char* string_value;
    /* Only used when the entry is "free". This points to the previous entry in
     * the free list. */
    struct NB_HandleMapEntry* prev;
  };
} NB_HandleMapEntry;

static NB_HandleMapEntry* s_nb_handle_map = NULL;
static size_t s_nb_handle_map_size = 0;
static size_t s_nb_handle_map_capacity = 0;
static NB_HandleMapEntry* s_nb_handle_map_free_head = NULL;

static inline NB_Bool nb_is_power_of_two(size_t n) {
  return (n & (n - 1)) == 0;
}

static inline NB_HashIndex nb_hash_handle(NB_Handle handle) {
  assert(nb_is_power_of_two(s_nb_handle_map_capacity));
  return (NB_HashIndex) (handle & (s_nb_handle_map_capacity - 1));
}

static inline NB_HandleMapEntry* nb_handle_main_entry(NB_Handle handle) {
  return &s_nb_handle_map[nb_hash_handle(handle)];
}

static inline NB_Bool nb_handle_entry_is_free(NB_HandleMapEntry* entry) {
  return entry->handle == NB_INVALID_HANDLE;
}

static NB_HandleMapEntry* nb_handle_new_entry(NB_Handle handle) {
  NB_HandleMapEntry* entry = nb_handle_main_entry(handle);
  if (!nb_handle_entry_is_free(entry)) {
    assert(s_nb_handle_map_free_head != NULL);
    NB_HandleMapEntry* free_entry = s_nb_handle_map_free_head;
    s_nb_handle_map_free_head = free_entry->next;
    if (free_entry->next) {
      free_entry->next->prev = NULL;
    }

    /* Our main position is already claimed. Check to see if the entry in that
     * position is in its main position */
    NB_HandleMapEntry* other_entry = nb_handle_main_entry(entry->handle);
    if (other_entry == entry) {
      /* Yes, so add this new entry to the chain, if it is not already there. */
      NB_HandleMapEntry* search = entry;
      while (search) {
        if (search->handle == handle) {
          NB_VERROR("handle %d is already registered.", handle);
          return NULL;
        }
        search = search->next;
      }

      /* Add as the second entry in the chain */
      free_entry->next = entry->next;
      entry->next = free_entry;
      entry = free_entry;
    } else {
      /* No, move the other entry to the free entry */
      assert(!nb_handle_entry_is_free(other_entry));
      while (other_entry->next != entry) {
        other_entry = other_entry->next;
      }

      other_entry->next = free_entry;
      memcpy(free_entry, entry, sizeof(NB_HandleMapEntry));

      entry->next = NULL;
    }
  } else {
    /* Remove from the free list */
    if (entry->next) {
      entry->next->prev = entry->prev;
    }
    if (entry->prev) {
      entry->prev->next = entry->next;
    } else {
      s_nb_handle_map_free_head = entry->next;
    }
    entry->next = NULL;
  }

  entry->handle = handle;
  return entry;
}

static NB_Bool nb_handle_map_resize(size_t new_capacity) {
  NB_VLOG(
      "Resizing handle map %u -> %u", s_nb_handle_map_capacity, new_capacity);
  assert(s_nb_handle_map_size <= new_capacity);

  NB_HandleMapEntry* old_map = s_nb_handle_map;
  size_t old_capacity = s_nb_handle_map_capacity;
  NB_HashIndex i;

  NB_HandleMapEntry* new_map = malloc(sizeof(NB_HandleMapEntry) * new_capacity);
  if (!new_map) {
    return NB_FALSE;
  }

  s_nb_handle_map = new_map;
  s_nb_handle_map_capacity = new_capacity;

  /* Update the free list */
  s_nb_handle_map_free_head = NULL;
  for (i = 0; i < s_nb_handle_map_capacity; ++i) {
    NB_HandleMapEntry* entry = &s_nb_handle_map[i];
    if (s_nb_handle_map_free_head) {
      s_nb_handle_map_free_head->prev = entry;
    }

    entry->handle = NB_INVALID_HANDLE;
    entry->next = s_nb_handle_map_free_head;
    s_nb_handle_map_free_head = entry;
  }
  s_nb_handle_map_free_head->prev = NULL;

  if (old_map) {
    /* Copy from old map to new map */
    for (i = 0; i < old_capacity; ++i) {
      if (nb_handle_entry_is_free(&old_map[i])) {
        continue;
      }

      NB_Handle handle = old_map[i].handle;
      NB_HandleMapEntry* entry = nb_handle_new_entry(handle);
      assert(entry != NULL);
      entry->type = old_map[i].type;
      entry->value = old_map[i].value;
      entry->string_value = old_map[i].string_value;
    }
  }

  free(old_map);
  return NB_TRUE;
}

static NB_Bool nb_register_handle(NB_Handle handle,
                                  NB_Type type,
                                  NB_HandleValue value) {
  if (!s_nb_handle_map) {
    if (!nb_handle_map_resize(NB_HANDLE_MAP_INITIAL_CAPACITY)) {
      return NB_FALSE;
    }
  }

  if (!s_nb_handle_map_free_head) {
    /* No more free space, allocate more */
    if (!nb_handle_map_resize(s_nb_handle_map_capacity * 2)) {
      return NB_FALSE;
    }
  }

  NB_HandleMapEntry* entry = nb_handle_new_entry(handle);
  if (!entry) {
    return NB_FALSE;
  }

  entry->type = type;
  entry->value = value;
  entry->string_value = NULL;
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

NB_Bool nb_handle_register_funcp(NB_Handle handle, void (*value)(void)) {
  NB_HandleValue hval;
  hval.funcp = value;
  return nb_register_handle(handle, NB_TYPE_FUNC_P, hval);
}

NB_Bool nb_handle_register_func_id(NB_Handle handle, int32_t value) {
  NB_HandleValue hval;
  hval.int32 = value;
  return nb_register_handle(handle, NB_TYPE_FUNC_ID, hval);
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

static NB_Bool nb_get_handle_entry(NB_Handle handle,
                                   NB_HandleMapEntry** out_entry) {
  NB_HandleMapEntry* entry = nb_handle_main_entry(handle);
  do {
    if (entry->handle == handle) {
      *out_entry = entry;
      return NB_TRUE;
    }

    entry = entry->next;
  } while(!nb_handle_entry_is_free(entry));

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

#define NB_HENTRY_FIELD(type) (hentry->value.type##_FIELD)

#define NB_TYPE_SWITCH(to_type, to)                 \
  switch (hentry->type) {                           \
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
    *out_value = NB_HENTRY_FIELD(from_type);       \
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
              nb_type_to_string(hentry->type),         \
              nb_type_to_string(to_type));             \
    return NB_FALSE /* no semicolon */

#define NB_CHECK(to_type, from_type)                \
  if (NB_HENTRY_FIELD(from_type) < to_type##_MIN || \
      NB_HENTRY_FIELD(from_type) > to_type##_MAX) { \
    NB_CHECK_ERROR(to_type, from_type);             \
    return NB_FALSE;                                \
  }

#define NB_CHECK_MAX(to_type, from_type)            \
  if (NB_HENTRY_FIELD(from_type) > to_type##_MAX) { \
    NB_CHECK_ERROR(to_type, from_type);             \
    return NB_FALSE;                                \
  }

#define NB_CHECK_GT_ZERO(to_type, from_type) \
  if (NB_HENTRY_FIELD(from_type) < 0) {      \
    NB_CHECK_ERROR(to_type, from_type);      \
    return NB_FALSE;                         \
  }

#define NB_CHECK_MAX_GT_ZERO(to_type, from_type)    \
  if (NB_HENTRY_FIELD(from_type) < 0 ||             \
      NB_HENTRY_FIELD(from_type) > to_type##_MAX) { \
    NB_CHECK_ERROR(to_type, from_type);             \
    return NB_FALSE;                                \
  }

#define NB_CHECK_SIGN(to_type, from_type) NB_CHECK(to_type, from_type)

#define NB_CHECK_FLT_TO_INT(to_type, from_type)                          \
  if (NB_HENTRY_FIELD(from_type) < to_type##_MIN ||                      \
      NB_HENTRY_FIELD(from_type) > to_type##_MAX ||                      \
      NB_HENTRY_FIELD(from_type) != rintf(NB_HENTRY_FIELD(from_type))) { \
    NB_CHECK_ERROR(to_type, from_type);                                  \
    return NB_FALSE;                                                     \
  }

#define NB_CHECK_FLOAT_TO_INT64(to_type, from_type)                      \
  if (NB_HENTRY_FIELD(from_type) < to_type##_MIN_FLOAT ||                \
      NB_HENTRY_FIELD(from_type) > to_type##_MAX_FLOAT ||                \
      NB_HENTRY_FIELD(from_type) != rintf(NB_HENTRY_FIELD(from_type))) { \
    NB_CHECK_ERROR(to_type, from_type);                                  \
    return NB_FALSE;                                                     \
  }

#define NB_CHECK_DBL_TO_INT(to_type, from_type)                         \
  if (NB_HENTRY_FIELD(from_type) < to_type##_MIN ||                     \
      NB_HENTRY_FIELD(from_type) > to_type##_MAX ||                     \
      NB_HENTRY_FIELD(from_type) != rint(NB_HENTRY_FIELD(from_type))) { \
    NB_CHECK_ERROR(to_type, from_type);                                 \
    return NB_FALSE;                                                    \
  }

#define NB_CHECK_INT_TO_FLT(to_type, from_type)            \
  if (NB_HENTRY_FIELD(from_type) < NB_TYPE_FLOAT_MIN_24 || \
      NB_HENTRY_FIELD(from_type) > NB_TYPE_FLOAT_MAX_24) { \
    NB_CHECK_ERROR(to_type, from_type);                    \
    return NB_FALSE;                                       \
  }

#define NB_CHECK_MAX_INT_TO_FLT(to_type, from_type)        \
  if (NB_HENTRY_FIELD(from_type) > NB_TYPE_FLOAT_MAX_24) { \
    NB_CHECK_ERROR(to_type, from_type);                    \
    return NB_FALSE;                                       \
  }

#define NB_CHECK_INT_TO_DBL(to_type, from_type)             \
  if (NB_HENTRY_FIELD(from_type) < NB_TYPE_DOUBLE_MIN_53 || \
      NB_HENTRY_FIELD(from_type) > NB_TYPE_DOUBLE_MAX_53) { \
    NB_CHECK_ERROR(to_type, from_type);                     \
    return NB_FALSE;                                        \
  }

#define NB_CHECK_MAX_INT_TO_DBL(to_type, from_type)         \
  if (NB_HENTRY_FIELD(from_type) > NB_TYPE_DOUBLE_MAX_53) { \
    NB_CHECK_ERROR(to_type, from_type);                     \
    return NB_FALSE;                                        \
  }

#define NB_CHECK_ERROR(to_type, from_type)              \
  NB_VERROR("handle %d(%s) with value " from_type##_FMT \
            " cannot be represented as %s.",            \
            handle,                                     \
            nb_type_to_string(hentry->type),            \
            NB_HENTRY_FIELD(from_type),                 \
            nb_type_to_string(to_type))

NB_Bool nb_handle_get_int8(NB_Handle handle, int8_t* out_value) {
  NB_HandleMapEntry* hentry;
  if (!nb_get_handle_entry(handle, &hentry)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_INT8, I8);
}

NB_Bool nb_handle_get_uint8(NB_Handle handle, uint8_t* out_value) {
  NB_HandleMapEntry* hentry;
  if (!nb_get_handle_entry(handle, &hentry)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_UINT8, U8);
}

NB_Bool nb_handle_get_int16(NB_Handle handle, int16_t* out_value) {
  NB_HandleMapEntry* hentry;
  if (!nb_get_handle_entry(handle, &hentry)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_INT16, I16);
}

NB_Bool nb_handle_get_uint16(NB_Handle handle, uint16_t* out_value) {
  NB_HandleMapEntry* hentry;
  if (!nb_get_handle_entry(handle, &hentry)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_UINT16, U16);
}

NB_Bool nb_handle_get_int32(NB_Handle handle, int32_t* out_value) {
  NB_HandleMapEntry* hentry;
  if (!nb_get_handle_entry(handle, &hentry)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_INT32, I32);
}

NB_Bool nb_handle_get_uint32(NB_Handle handle, uint32_t* out_value) {
  NB_HandleMapEntry* hentry;
  if (!nb_get_handle_entry(handle, &hentry)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_UINT32, U32);
}

NB_Bool nb_handle_get_int64(NB_Handle handle, int64_t* out_value) {
  NB_HandleMapEntry* hentry;
  if (!nb_get_handle_entry(handle, &hentry)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_INT64, I64);
}

NB_Bool nb_handle_get_uint64(NB_Handle handle, uint64_t* out_value) {
  NB_HandleMapEntry* hentry;
  if (!nb_get_handle_entry(handle, &hentry)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_UINT64, U64);
}

NB_Bool nb_handle_get_float(NB_Handle handle, float* out_value) {
  NB_HandleMapEntry* hentry;
  if (!nb_get_handle_entry(handle, &hentry)) {
    return NB_FALSE;
  }

  NB_TYPE_SWITCH(NB_TYPE_FLOAT, FLT);
}

NB_Bool nb_handle_get_double(NB_Handle handle, double* out_value) {
  NB_HandleMapEntry* hentry;
  if (!nb_get_handle_entry(handle, &hentry)) {
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
#undef NB_HENTRY_FIELD
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

static NB_Bool nb_hentry_string_value(NB_HandleMapEntry* hentry,
                                      char** out_value) {
  if (hentry->string_value == NULL) {
    uint32_t len;
    const char* str;
    if (!nb_var_string(hentry->value.var, &str, &len)) {
      return NB_FALSE;
    }

    hentry->string_value = strndup(str, len);
  }

  *out_value = hentry->string_value;
  return NB_TRUE;
}

NB_Bool nb_handle_get_voidp(NB_Handle handle, void** out_value) {
  NB_HandleMapEntry* hentry;
  if (!nb_get_handle_entry(handle, &hentry)) {
    return NB_FALSE;
  }

  if (hentry->type == NB_TYPE_VAR) {
    char* string_value;
    if (!nb_hentry_string_value(hentry, &string_value)) {
      NB_VERROR("unable to get string for handle %d", handle);
      return NB_FALSE;
    }
    *out_value = string_value;
  } else if (hentry->type == NB_TYPE_VOID_P) {
    *out_value = hentry->value.voidp;
  } else {
    NB_VERROR("handle %d is of type %s. Expected %s.",
              handle,
              nb_type_to_string(hentry->type),
              nb_type_to_string(NB_TYPE_VOID_P));
    return NB_FALSE;
  }

  return NB_TRUE;
}

NB_Bool nb_handle_get_funcp(NB_Handle handle, void (**out_value)(void)) {
  NB_HandleMapEntry* hentry;
  if (!nb_get_handle_entry(handle, &hentry)) {
    return NB_FALSE;
  }

  if (hentry->type != NB_TYPE_FUNC_P) {
    NB_VERROR("handle %d is of type %s. Expected %s.",
              handle,
              nb_type_to_string(hentry->type),
              nb_type_to_string(NB_TYPE_FUNC_P));
    return NB_FALSE;
  }

  *out_value = hentry->value.funcp;
  return NB_TRUE;
}

NB_Bool nb_handle_get_func_id(NB_Handle handle, int32_t* out_value) {
  NB_HandleMapEntry* hentry;
  if (!nb_get_handle_entry(handle, &hentry)) {
    return NB_FALSE;
  }

  if (hentry->type != NB_TYPE_FUNC_ID) {
    NB_VERROR("handle %d is of type %s. Expected %s.",
              handle,
              nb_type_to_string(hentry->type),
              nb_type_to_string(NB_TYPE_FUNC_ID));
    return NB_FALSE;
  }

  *out_value = hentry->value.int32;
  return NB_TRUE;
}

NB_Bool nb_handle_get_charp(NB_Handle handle, char** out_value) {
  NB_HandleMapEntry* hentry;
  if (!nb_get_handle_entry(handle, &hentry)) {
    return NB_FALSE;
  }

  if (hentry->type == NB_TYPE_VAR) {
    char* string_value;
    if (!nb_hentry_string_value(hentry, &string_value)) {
      NB_VERROR("unable to get string for handle %d", handle);
      return NB_FALSE;
    }
    *out_value = string_value;
  } else if (hentry->type == NB_TYPE_VOID_P) {
    *out_value = (char*)hentry->value.voidp;
  } else {
    NB_VERROR("handle %d is of type %s. Expected %s.",
              handle,
              nb_type_to_string(hentry->type),
              nb_type_to_string(NB_TYPE_VOID_P));
    return NB_FALSE;
  }

  return NB_TRUE;
}

NB_Bool nb_handle_get_var(NB_Handle handle, struct PP_Var* out_value) {
  NB_HandleMapEntry* hentry;
  if (!nb_get_handle_entry(handle, &hentry)) {
    return NB_FALSE;
  }

  if (hentry->type != NB_TYPE_VAR) {
    NB_VERROR("handle %d is of type %s. Expected %s.",
              handle,
              nb_type_to_string(hentry->type),
              nb_type_to_string(NB_TYPE_VAR));
    return NB_FALSE;
  }

  *out_value = hentry->value.var;
  return NB_TRUE;
}

NB_Bool nb_handle_get_default(NB_Handle handle,
                              NB_VarArgInt** iargs,
                              NB_VarArgInt* max_iargs,
                              NB_VarArgDbl** dargs,
                              NB_VarArgDbl* max_dargs) {
  NB_HandleMapEntry* hentry;
  if (!nb_get_handle_entry(handle, &hentry)) {
    return NB_FALSE;
  }

#define NB_CHECK(args, max_args, needed, type) \
  if (args + needed - 1 >= max_args) {         \
    NB_ERROR("Too many " type "args passed."); \
    return NB_FALSE;                           \
  }

#ifdef __x86_64__
#define NB_PUSH_INT(field)                     \
  NB_CHECK(*iargs, max_iargs, 1, "non-float ") \
  *(*iargs)++ = (uint64_t)(int64_t)hentry->value.field

#define NB_PUSH_UINT(field)                    \
  NB_CHECK(*iargs, max_iargs, 1, "non-float ") \
  *(*iargs)++ = (uint64_t)hentry->value.field

#define NB_PUSH_INT64(field) NB_PUSH_INT(field)
#define NB_PUSH_UINT64(field) NB_PUSH_UINT(field)

#define NB_PUSH_DOUBLE(field)              \
  NB_CHECK(*dargs, max_dargs, 1, "float ") \
  *(*dargs)++ = (double)hentry->value.field

#define NB_PUSH_VOIDP(field)                   \
  NB_CHECK(*iargs, max_iargs, 1, "non-float ") \
  *(*iargs)++ = (uint64_t)(uint32_t)hentry->value.field

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

#define NB_PUSH_INT(field)                                          \
  NB_CHECK(*iargs, max_iargs, 1, "") x.int32 = hentry->value.field; \
  *(*iargs)++ = x.lo

#define NB_PUSH_UINT(field)                                          \
  NB_CHECK(*iargs, max_iargs, 1, "") x.uint32 = hentry->value.field; \
  *(*iargs)++ = x.lo

#define NB_PUSH_INT64(field)                                        \
  NB_CHECK(*iargs, max_iargs, 2, "") x.int64 = hentry->value.field; \
  *(*iargs)++ = x.lo;                                               \
  *(*iargs)++ = x.hi

#define NB_PUSH_UINT64(field)                                        \
  NB_CHECK(*iargs, max_iargs, 2, "") x.uint64 = hentry->value.field; \
  *(*iargs)++ = x.lo;                                                \
  *(*iargs)++ = x.hi

#define NB_PUSH_DOUBLE(field)                                         \
  NB_CHECK(*iargs, max_iargs, 2, "") x.float64 = hentry->value.field; \
  *(*iargs)++ = x.lo;                                                 \
  *(*iargs)++ = x.hi

#define NB_PUSH_VOIDP(field)         \
  NB_CHECK(*iargs, max_iargs, 1, "") \
  *(*iargs)++ = (uint32_t)hentry->value.field
#endif

  switch (hentry->type) {
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
                nb_type_to_string(hentry->type));
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
  NB_HandleMapEntry* entry;
  if (!nb_get_handle_entry(handle, &entry)) {
    NB_VERROR("Destroying handle %d, but it doesn't exist.", handle);
    return;
  }

  /* Destroy resources associated with this handle */
  if (entry->type == NB_TYPE_VAR) {
    nb_var_release(entry->value.var);
  }
  free(entry->string_value);
  entry->string_value = NULL;

  /* Remove from chain */
  NB_HandleMapEntry* search = nb_handle_main_entry(handle);
  assert(search != NULL);
  if (search != entry) {
    while (search->next != entry) {
      search = search->next;
    }
    search->next = entry->next;
  } else {
    /* Removing the top of the chain; move the next element up, if one exists */
    NB_HandleMapEntry* next_entry = search->next;
    if (next_entry != NULL) {
      memcpy(entry, next_entry, sizeof(NB_HandleMapEntry));
      entry = next_entry;
    }
  }

  if (entry) {
    /* Add to free list */
    if (s_nb_handle_map_free_head != NULL) {
      s_nb_handle_map_free_head->prev = entry;
    }
    entry->handle = NB_INVALID_HANDLE;
    entry->next = s_nb_handle_map_free_head;
    entry->prev = NULL;
    s_nb_handle_map_free_head = entry;
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
  NB_HandleMapEntry* hentry;
  if (!nb_get_handle_entry(handle, &hentry)) {
    return NB_FALSE;
  }

  switch (hentry->type) {
    case NB_TYPE_INT8:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hentry->value.int8;
      break;
    case NB_TYPE_UINT8:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hentry->value.uint8;
      break;
    case NB_TYPE_INT16:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hentry->value.int16;
      break;
    case NB_TYPE_UINT16:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hentry->value.uint16;
      break;
    case NB_TYPE_INT32:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hentry->value.int32;
      break;
    case NB_TYPE_UINT32:
      var->type = PP_VARTYPE_INT32;
      var->value.as_int = hentry->value.uint32;
      break;
    case NB_TYPE_INT64:
      *var = nb_var_int64_create(hentry->value.int64);
      break;
    case NB_TYPE_UINT64:
      *var = nb_var_int64_create((int64_t)hentry->value.uint64);
      break;
    case NB_TYPE_FLOAT:
      var->type = PP_VARTYPE_DOUBLE;
      var->value.as_double = hentry->value.float32;
      break;
    case NB_TYPE_DOUBLE:
      var->type = PP_VARTYPE_DOUBLE;
      var->value.as_double = hentry->value.float64;
      break;
    case NB_TYPE_VAR:
      *var = hentry->value.var;
      nb_var_addref(*var);
      break;
    case NB_TYPE_VOID_P:
      if (hentry->value.voidp) {
        var->type = PP_VARTYPE_INT32;
        var->value.as_int = handle;
      } else {
        var->type = PP_VARTYPE_NULL;
      }
      break;
    default:
      NB_VERROR("Don't know how to convert handle %d with type %s to var",
                handle,
                nb_type_to_string(hentry->type));
      return NB_FALSE;
  }

  return NB_TRUE;
}
