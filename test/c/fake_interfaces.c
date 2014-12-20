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

#include "fake_interfaces.h"
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ppapi/c/pp_var.h>
#include <ppapi/c/ppb_var.h>
#include <ppapi/c/ppb_var_array.h>
#include <ppapi/c/ppb_var_array_buffer.h>
#include <ppapi/c/ppb_var_dictionary.h>
#include <ppapi/c/ppb_messaging.h>
#include "bool.h"
#include "error.h"
#include "json.h"
#include "var.h"

#define FAKE_INTERFACE_TRACE 0
enum { kArrayInitCount = 4 };
enum { kDictInitCount = 4 };
enum { kDataCap = 1024 };

struct VarString {
  char* data;
  uint32_t len;
};

struct VarArray {
  struct PP_Var* data;
  uint32_t len;
  uint32_t cap;
};

struct VarBuffer {
  uint8_t* data;
  uint32_t len;
};

struct VarDict {
  struct PP_Var* keys;
  struct PP_Var* values;
  uint32_t len;
  uint32_t cap;
};

struct VarData {
  PP_VarType type;
  int32_t ref_count;
  union {
    struct VarString string;
    struct VarArray array;
    struct VarBuffer buffer;
    struct VarDict dict;
    int next_free;
  };
};

#define FAKE_INTERFACE_LOCK                                \
  do {                                                     \
    if (pthread_mutex_lock(&s_fake_interface_lock) != 0) { \
      NB_ERROR("pthread_mutex_lock failed.");              \
      exit(1);                                             \
    }                                                      \
  } while (0)

#define FAKE_INTERFACE_UNLOCK                                \
  do {                                                       \
    if (pthread_mutex_unlock(&s_fake_interface_lock) != 0) { \
      NB_ERROR("pthread_mutex_unlock_failed.");              \
      exit(1);                                               \
    }                                                        \
  } while (0)

#define VAR_DEBUG_STRING(var, size) \
  char var##_str[size];             \
  var_debug_string_locked(var, var##_str, size) /* no semicolon */

/* Static variables */
static struct VarData s_data[kDataCap];
static int s_data_first_free = 0;
static PostMessageCallback s_post_message_callback;
static void* s_post_message_callback_user_data;

static pthread_mutex_t s_fake_interface_lock = PTHREAD_MUTEX_INITIALIZER;

/* Forward declarations for the interfaces */
static void var_data_init_all(void);
static void var_data_destroy_all(void);

static struct VarData* var_data_get_locked(int64_t id);
static struct VarData* var_data_get_type_locked(struct PP_Var var,
                                                PP_VarType type);
static struct VarData* var_data_alloc_locked(void);
static void var_data_destroy_locked(struct VarData* var_data);
static void var_data_free_locked(int64_t id);

void var_debug_string_locked(struct PP_Var, char* buf, size_t buf_size);
static void var_add_ref(struct PP_Var);
static void var_release(struct PP_Var);
static void var_add_ref_locked(struct PP_Var);
static void var_release_locked(struct PP_Var);

static struct PP_Var var_from_utf8(const char* data, uint32_t len);
static const char* var_to_utf8(struct PP_Var, uint32_t* len);
static const char* var_to_utf8_locked(struct PP_Var, uint32_t* len);
static PP_Bool var_string_equal_locked(struct PP_Var var1, struct PP_Var var2);

static struct PP_Var array_create(void);
static struct PP_Var array_create_locked(void);
static struct PP_Var array_get(struct PP_Var, uint32_t index);
static PP_Bool array_set(struct PP_Var, uint32_t index, struct PP_Var value);
static PP_Bool array_set_locked(struct PP_Var,
                                uint32_t index,
                                struct PP_Var value);
static uint32_t array_get_length(struct PP_Var);
static PP_Bool array_set_length(struct PP_Var, uint32_t length);

static struct PP_Var buffer_create(uint32_t size_in_bytes);
static PP_Bool buffer_byte_length(struct PP_Var, uint32_t* byte_length);
static void* buffer_map(struct PP_Var);
static void buffer_unmap(struct PP_Var);

static struct PP_Var dict_create(void);
static PP_Bool dict_find_locked(struct PP_Var var,
                                struct PP_Var key,
                                struct VarData** out_var_data,
                                uint32_t* out_index);
static struct PP_Var dict_get(struct PP_Var, struct PP_Var key);
static PP_Bool dict_set(struct PP_Var, struct PP_Var key, struct PP_Var value);
static void dict_delete(struct PP_Var, struct PP_Var key);
static PP_Bool dict_has_key(struct PP_Var, struct PP_Var key);
static struct PP_Var dict_get_keys(struct PP_Var);

static void messaging_post_message(PP_Instance instance, struct PP_Var message);

static struct PPB_Var_1_1 s_ppb_var = {
    &var_add_ref,
    &var_release,
    &var_from_utf8,
    &var_to_utf8,
};

static struct PPB_VarArray_1_0 s_ppb_var_array = {
    &array_create,
    &array_get,
    &array_set,
    &array_get_length,
    &array_set_length,
};

static struct PPB_VarArrayBuffer_1_0 s_ppb_var_array_buffer = {
    &buffer_create,
    &buffer_byte_length,
    &buffer_map,
    &buffer_unmap,
};

static struct PPB_VarDictionary_1_0 s_ppb_var_dict = {
    &dict_create,
    &dict_get,
    &dict_set,
    &dict_delete,
    &dict_has_key,
    &dict_get_keys,
};

static struct PPB_Messaging_1_0 s_ppb_messaging = {
    &messaging_post_message,
};

void fake_interface_init(void) {
  var_data_destroy_all();
  var_data_init_all();
}

void fake_interface_destroy(void) {
  var_data_destroy_all();
}

void fake_interface_set_post_message_callback(PostMessageCallback callback,
                                              void* user_data) {
  FAKE_INTERFACE_LOCK;
  s_post_message_callback = callback;
  s_post_message_callback_user_data = user_data;
  FAKE_INTERFACE_UNLOCK;
}

NB_Bool fake_interface_check_no_references(void) {
  NB_Bool result = NB_TRUE;
  int i;
  for (i = 0; i < kDataCap; ++i) {
    struct PP_Var var;
    char* json = NULL;

    if (s_data[i].ref_count == 0) {
      continue;
    }

    var.type = s_data[i].type;
    var.value.as_id = i;
    json = var_to_json(var);

    NB_VERROR("VarData with id=%d, type=\"%s\" has non-zero refcount %d:\n%s",
              i, nb_var_type_to_string(s_data[i].type), s_data[i].ref_count,
              json);
    result = NB_FALSE;
  }

  return result;
}

const void* fake_get_browser_interface(const char* interface_name) {
  if (strcmp(interface_name, PPB_VAR_INTERFACE_1_1) == 0) {
    return &s_ppb_var;
  } else if (strcmp(interface_name, PPB_VAR_ARRAY_INTERFACE_1_0) == 0) {
    return &s_ppb_var_array;
  } else if (strcmp(interface_name, PPB_VAR_ARRAY_BUFFER_INTERFACE_1_0) == 0) {
    return &s_ppb_var_array_buffer;
  } else if (strcmp(interface_name, PPB_VAR_DICTIONARY_INTERFACE_1_0) == 0) {
    return &s_ppb_var_dict;
  } else if (strcmp(interface_name, PPB_MESSAGING_INTERFACE_1_0) == 0) {
    return &s_ppb_messaging;
  } else {
    assert(!"Unknown interface name");
  }
}

void var_data_init_all(void) {
  int i;
  for (i = 0; i < kDataCap; ++i) {
    s_data[i].type = PP_VARTYPE_UNDEFINED;
    s_data[i].ref_count = 0;
    s_data[i].next_free = i == kDataCap - 1 ? -1 : i + 1;
  }
}

void var_data_destroy_all(void) {
  int i;
  for (i = 0; i < kDataCap; ++i) {
    var_data_destroy_locked(&s_data[i]);
  }
}

struct VarData* var_data_get_locked(int64_t id) {
  if (id < 0 || id >= kDataCap) {
    return NULL;
  }

  struct VarData* result = &s_data[id];
  if (result->ref_count == 0) {
    NB_VERROR("var_data_get(%lld) called with unallocated id.", id);
    return NULL;
  }

  return &s_data[id];
}

struct VarData* var_data_get_type_locked(struct PP_Var var, PP_VarType type) {
  struct VarData* var_data;
  if (!nb_var_check_type_with_error(var, type)) {
    return NULL;
  }

  var_data = var_data_get_locked(var.value.as_id);
  if (var_data == NULL) {
    NB_VERROR("var_data_get_type(%lld) called with bad id.", var.value.as_id);
    return NULL;
  }

  if (var_data->type != var.type) {
    NB_VERROR(
        "var_data_get_type(%lld) called with mismatching type:"
        " var.type = %s, var_data->type = %s.",
        var.value.as_id, nb_var_type_to_string(var.type),
        nb_var_type_to_string(var_data->type));
    return NULL;
  }

  return var_data;
}

struct VarData* var_data_alloc_locked(void) {
  struct VarData* var_data;
  if (s_data_first_free == -1) {
    NB_ERROR("var_data_alloc() failed.");
    return NULL;
  }

  var_data = &s_data[s_data_first_free];
  assert(var_data->ref_count == 0);

  s_data_first_free = var_data->next_free;

#if FAKE_INTERFACE_TRACE > 1
  NB_VERROR("var_data_alloc() => %d", var_data - &s_data[0]);
#endif

  return var_data;
}

void var_data_destroy_locked(struct VarData* var_data) {
  uint32_t i;
  switch (var_data->type) {
    case PP_VARTYPE_STRING:
      free(var_data->string.data);
      break;

    case PP_VARTYPE_ARRAY:
      for (i = 0; i < var_data->array.len; ++i) {
        var_release_locked(var_data->array.data[i]);
      }
      free(var_data->array.data);
      break;

    case PP_VARTYPE_DICTIONARY:
      for (i = 0; i < var_data->dict.len; ++i) {
        var_release_locked(var_data->dict.keys[i]);
        var_release_locked(var_data->dict.values[i]);
      }
      free(var_data->dict.keys);
      free(var_data->dict.values);
      break;

    case PP_VARTYPE_ARRAY_BUFFER:
      free(var_data->buffer.data);
      break;

    default:
      return;
  }
}

void var_data_free_locked(int64_t id) {
  assert(id >= 0 && id < kDataCap);

  struct VarData* var_data = &s_data[id];
  assert(var_data->ref_count == 0);

#if FAKE_INTERFACE_TRACE > 1
  NB_VERROR("var_data_free(<%s: %lld>)", nb_var_type_to_string(var_data->type),
            id);
#endif

  var_data_destroy_locked(var_data);

  var_data->type = PP_VARTYPE_UNDEFINED;
  var_data->next_free = s_data_first_free;
  s_data_first_free = id;
}

void var_debug_string_locked(struct PP_Var var, char* buf, size_t buf_size) {
  switch (var.type) {
    case PP_VARTYPE_UNDEFINED:
      strncpy(buf, "undefined", buf_size);
      break;
    case PP_VARTYPE_NULL:
      strncpy(buf, "null", buf_size);
      break;
    case PP_VARTYPE_BOOL:
      strncpy(buf, var.value.as_bool ? "true" : "false", buf_size);
      break;
    case PP_VARTYPE_INT32:
      snprintf(buf, buf_size, "%d", var.value.as_int);
      break;
    case PP_VARTYPE_DOUBLE:
      snprintf(buf, buf_size, "%g", var.value.as_double);
      break;
    case PP_VARTYPE_STRING: {
      uint32_t len;
      const char* str = var_to_utf8_locked(var, &len);
      snprintf(buf, buf_size, "\"%.*s\"", len, str);
      break;
    }
    case PP_VARTYPE_OBJECT:
      strncpy(buf, "<object>", buf_size);
      break;
    case PP_VARTYPE_ARRAY:
    case PP_VARTYPE_DICTIONARY:
    case PP_VARTYPE_ARRAY_BUFFER:
    case PP_VARTYPE_RESOURCE:
      snprintf(buf, buf_size, "<%s: %lld>", nb_var_type_to_string(var.type),
               var.value.as_id);
      break;
    default:
      strncpy(buf, "<unknown>", buf_size);
      break;
  }
}

void var_add_ref(struct PP_Var var) {
  FAKE_INTERFACE_LOCK;
  var_add_ref_locked(var);
  FAKE_INTERFACE_UNLOCK;
}

void var_release(struct PP_Var var) {
  FAKE_INTERFACE_LOCK;
  var_release_locked(var);
  FAKE_INTERFACE_UNLOCK;
}

void var_add_ref_locked(struct PP_Var var) {
  struct VarData* var_data;
  switch (var.type) {
    case PP_VARTYPE_STRING:
    case PP_VARTYPE_ARRAY:
    case PP_VARTYPE_DICTIONARY:
    case PP_VARTYPE_ARRAY_BUFFER:
      break;
    default:
      return;
  }

  var_data = var_data_get_locked(var.value.as_id);
  if (var_data == NULL) {
    NB_VERROR("var_add_ref(%lld) called with bad id.", var.value.as_id);
    return;
  }

#if FAKE_INTERFACE_TRACE > 1
  NB_VERROR("var_add_ref(%lld) rc:%d->%d", var.value.as_id, var_data->ref_count,
            var_data->ref_count + 1);
#endif

  var_data->ref_count++;
}

void var_release_locked(struct PP_Var var) {
  struct VarData* var_data;
  int ref_count;
  switch (var.type) {
    case PP_VARTYPE_STRING:
    case PP_VARTYPE_ARRAY:
    case PP_VARTYPE_DICTIONARY:
    case PP_VARTYPE_ARRAY_BUFFER:
      break;
    default:
      return;
  }

  var_data = var_data_get_locked(var.value.as_id);
  if (var_data == NULL) {
    NB_VERROR("var_release(%lld) called with bad id.", var.value.as_id);
    return;
  }

#if FAKE_INTERFACE_TRACE > 1
  NB_VERROR("var_release(%lld) rc:%d->%d", var.value.as_id, var_data->ref_count,
            var_data->ref_count - 1);
#endif

  ref_count = --var_data->ref_count;
  if (ref_count == 0) {
    var_data_free_locked(var.value.as_id);
  } else if (ref_count < 0) {
    NB_VERROR("var_release(%lld) called with <=0 ref_count: %d.",
              var.value.as_id, ref_count + 1);
  }
}

struct PP_Var var_from_utf8(const char* data, uint32_t len) {
  FAKE_INTERFACE_LOCK;

  struct PP_Var result;
  struct VarData* var_data = var_data_alloc_locked();
  assert(var_data != NULL);

  var_data->type = PP_VARTYPE_STRING;
  var_data->ref_count = 1;
  var_data->string.data = malloc(len);
  memcpy(var_data->string.data, data, len);
  var_data->string.len = len;

  memset(&result, 0, sizeof(struct PP_Var));
  result.type = PP_VARTYPE_STRING;
  result.value.as_id = var_data - s_data;

#if FAKE_INTERFACE_TRACE > 0
  NB_VERROR("var_from_utf8(\"%s\", %u) => %lld", data, len, result.value.as_id);
#endif

  FAKE_INTERFACE_UNLOCK;
  return result;
}

const char* var_to_utf8(struct PP_Var var, uint32_t* len) {
  FAKE_INTERFACE_LOCK;
  const char* result = var_to_utf8_locked(var, len);
  FAKE_INTERFACE_UNLOCK;
  return result;
}

const char* var_to_utf8_locked(struct PP_Var var, uint32_t* len) {
  assert(len != NULL);
  struct VarData* var_data = var_data_get_type_locked(var, PP_VARTYPE_STRING);
  if (var_data == NULL) {
    return NULL;
  }

  *len = var_data->string.len;

#if FAKE_INTERFACE_TRACE > 0
  NB_VERROR("var_to_utf8(%lld) => (\"%.*s\", %u)", var.value.as_id, *len,
            var_data->string.data, *len);
#endif

  return var_data->string.data;
}

PP_Bool var_string_equal_locked(struct PP_Var var1, struct PP_Var var2) {
  uint32_t len1;
  uint32_t len2;
  const char* str1;
  const char* str2;
  assert(var1.type == PP_VARTYPE_STRING);
  assert(var2.type == PP_VARTYPE_STRING);
  str1 = var_to_utf8_locked(var1, &len1);
  str2 = var_to_utf8_locked(var2, &len2);
  if (len1 != len2) {
    return PP_FALSE;
  }

  return memcmp(str1, str2, len1) == 0;
}

struct PP_Var array_create(void) {
  FAKE_INTERFACE_LOCK;
  struct PP_Var result = array_create_locked();
  FAKE_INTERFACE_UNLOCK;
  return result;
}

struct PP_Var array_create_locked(void) {
  struct PP_Var result;
  struct VarData* var_data = var_data_alloc_locked();
  assert(var_data != NULL);

  var_data->type = PP_VARTYPE_ARRAY;
  var_data->ref_count = 1;
  var_data->array.data = calloc(kArrayInitCount, sizeof(struct PP_Var));
  var_data->array.len = 0;
  var_data->array.cap = kArrayInitCount;

  memset(&result, 0, sizeof(struct PP_Var));
  result.type = PP_VARTYPE_ARRAY;
  result.value.as_id = var_data - s_data;

#if FAKE_INTERFACE_TRACE > 0
  NB_VERROR("array_create => %lld", result.value.as_id);
#endif

  return result;
}

struct PP_Var array_get(struct PP_Var var, uint32_t index) {
  FAKE_INTERFACE_LOCK;
  struct PP_Var result = PP_MakeUndefined();
  struct VarData* var_data = var_data_get_type_locked(var, PP_VARTYPE_ARRAY);
  if (var_data == NULL) {
    FAKE_INTERFACE_UNLOCK;
    return result;
  }

  if (index >= var_data->array.len) {
    FAKE_INTERFACE_UNLOCK;
    return result;
  }

  result = var_data->array.data[index];
  var_add_ref_locked(result);

#if FAKE_INTERFACE_TRACE > 0
  VAR_DEBUG_STRING(var, 256);
  VAR_DEBUG_STRING(result, 256);
  NB_VERROR("array_get(%s, %u) => %s", var_str, index, result_str);
#endif

  FAKE_INTERFACE_UNLOCK;
  return result;
}

PP_Bool array_set(struct PP_Var var, uint32_t index, struct PP_Var value) {
  FAKE_INTERFACE_LOCK;
  PP_Bool result = array_set_locked(var, index, value);
  FAKE_INTERFACE_UNLOCK;
  return result;
}

PP_Bool array_set_locked(struct PP_Var var,
                         uint32_t index,
                         struct PP_Var value) {
  struct VarData* var_data = var_data_get_type_locked(var, PP_VARTYPE_ARRAY);
  if (var_data == NULL) {
#if FAKE_INTERFACE_TRACE > 0
    VAR_DEBUG_STRING(var, 256);
    NB_VERROR("array_set(%s) FAILED", var_str);
#endif

    return PP_FALSE;
  }

  if (index >= var_data->array.cap) {
    uint32_t new_cap = index * 2;
    size_t new_size = new_cap * sizeof(struct PP_Var);
    struct PP_Var* new_data = realloc(var_data->array.data, new_size);
    assert(new_data != NULL);

    var_data->array.data = new_data;
    var_data->array.cap = new_cap;
  }

  if (index >= var_data->array.len) {
    int new_len = index + 1;
    int i;
    for (i = var_data->array.len; i < new_len - 1; ++i) {
      var_data->array.data[i] = PP_MakeUndefined();
    }

    var_data->array.len = new_len;
  }

  var_add_ref_locked(value);
  var_release_locked(var_data->array.data[index]);
  var_data->array.data[index] = value;

#if FAKE_INTERFACE_TRACE > 0
  VAR_DEBUG_STRING(var, 256);
  VAR_DEBUG_STRING(value, 256);
  NB_VERROR("array_set(%s, %u, %s)", var_str, index, value_str);
#endif

  return PP_TRUE;
}

uint32_t array_get_length(struct PP_Var var) {
  FAKE_INTERFACE_LOCK;
  struct VarData* var_data = var_data_get_type_locked(var, PP_VARTYPE_ARRAY);
  if (var_data == NULL) {
#if FAKE_INTERFACE_TRACE > 0
    VAR_DEBUG_STRING(var, 256);
    NB_VERROR("array_get_length(%s) FAILED", var_str);
#endif

    FAKE_INTERFACE_UNLOCK;
    return 0;
  }

  uint32_t result = var_data->array.len;

#if FAKE_INTERFACE_TRACE > 0
  VAR_DEBUG_STRING(var, 256);
  NB_VERROR("array_get_length(%s) => %u", var_str, result);
#endif

  FAKE_INTERFACE_UNLOCK;
  return result;
}

PP_Bool array_set_length(struct PP_Var var, uint32_t length) {
  FAKE_INTERFACE_LOCK;
  struct VarData* var_data;
  uint32_t new_cap;
  size_t new_size;
  struct PP_Var* new_data;
  int i;

  var_data = var_data_get_type_locked(var, PP_VARTYPE_ARRAY);
  if (var_data == NULL) {
#if FAKE_INTERFACE_TRACE > 0
    VAR_DEBUG_STRING(var, 256);
    NB_VERROR("array_set_length(%s, %u) FAILED", var_str, length);
#endif

    FAKE_INTERFACE_UNLOCK;
    return PP_FALSE;
  }

  new_cap = length;
  new_size = new_cap * sizeof(struct PP_Var);
  new_data = realloc(var_data->array.data, new_size);
  assert(new_data != NULL);

  var_data->array.data = new_data;
  var_data->array.cap = new_cap;

  if (length > var_data->array.len) {
    for (i = var_data->array.len; i < length; ++i) {
      var_data->array.data[i] = PP_MakeUndefined();
    }
  } else if (length < var_data->array.len) {
    for (i = length; i < var_data->array.len; ++i) {
      var_release_locked(var_data->array.data[i]);
    }
  }

  var_data->array.len = length;

#if FAKE_INTERFACE_TRACE > 0
  VAR_DEBUG_STRING(var, 256);
  NB_VERROR("array_set_length(%s, %u)", var_str, length);
#endif

  FAKE_INTERFACE_UNLOCK;
  return PP_TRUE;
}

struct PP_Var buffer_create(uint32_t size_in_bytes) {
  FAKE_INTERFACE_LOCK;
  struct PP_Var result;
  struct VarData* var_data = var_data_alloc_locked();
  assert(var_data != NULL);

  var_data->type = PP_VARTYPE_ARRAY_BUFFER;
  var_data->ref_count = 1;
  var_data->buffer.data = calloc(size_in_bytes, 1);
  var_data->buffer.len = size_in_bytes;

  memset(&result, 0, sizeof(struct PP_Var));
  result.type = PP_VARTYPE_ARRAY_BUFFER;
  result.value.as_id = var_data - s_data;

#if FAKE_INTERFACE_TRACE > 0
  NB_VERROR("buffer_create => %lld", result.value.as_id);
#endif

  FAKE_INTERFACE_UNLOCK;
  return result;
}

PP_Bool buffer_byte_length(struct PP_Var var, uint32_t* byte_length) {
  FAKE_INTERFACE_LOCK;
  struct VarData* var_data =
      var_data_get_type_locked(var, PP_VARTYPE_ARRAY_BUFFER);
  if (var_data == NULL) {
    FAKE_INTERFACE_UNLOCK;
    return PP_FALSE;
  }

  *byte_length = var_data->buffer.len;

#if FAKE_INTERFACE_TRACE > 0
  VAR_DEBUG_STRING(var, 256);
  NB_VERROR("buffer_byte_length(%s) => %u", var_str, *byte_length);
#endif

  FAKE_INTERFACE_UNLOCK;
  return PP_TRUE;
}

void* buffer_map(struct PP_Var var) {
  FAKE_INTERFACE_LOCK;
  struct VarData* var_data =
      var_data_get_type_locked(var, PP_VARTYPE_ARRAY_BUFFER);
  if (var_data == NULL) {
    FAKE_INTERFACE_UNLOCK;
    return NULL;
  }

#if FAKE_INTERFACE_TRACE > 0
  VAR_DEBUG_STRING(var, 256);
  NB_VERROR("buffer_map(%s)", var_str);
#endif

  FAKE_INTERFACE_UNLOCK;
  return var_data->buffer.data;
}

void buffer_unmap(struct PP_Var var) {
  FAKE_INTERFACE_LOCK;
  // Call just for the type checks.
  var_data_get_type_locked(var, PP_VARTYPE_ARRAY_BUFFER);

#if FAKE_INTERFACE_TRACE > 0
  VAR_DEBUG_STRING(var, 256);
  NB_VERROR("buffer_unmap(%s)", var_str);
#endif

  FAKE_INTERFACE_UNLOCK;
}

struct PP_Var dict_create(void) {
  FAKE_INTERFACE_LOCK;
  struct PP_Var result;
  struct VarData* var_data = var_data_alloc_locked();
  assert(var_data != NULL);

  var_data->type = PP_VARTYPE_DICTIONARY;
  var_data->ref_count = 1;
  var_data->dict.keys = calloc(kDictInitCount, sizeof(struct PP_Var));
  var_data->dict.values = calloc(kDictInitCount, sizeof(struct PP_Var));
  var_data->dict.len = 0;
  var_data->dict.cap = kDictInitCount;

  memset(&result, 0, sizeof(struct PP_Var));
  result.type = PP_VARTYPE_DICTIONARY;
  result.value.as_id = var_data - s_data;

#if FAKE_INTERFACE_TRACE > 0
  NB_VERROR("dict_create => %lld", result.value.as_id);
#endif

  FAKE_INTERFACE_UNLOCK;
  return result;
}

PP_Bool dict_find_locked(struct PP_Var var,
                         struct PP_Var key,
                         struct VarData** out_var_data,
                         uint32_t* out_index) {
  uint32_t i;
  struct VarData* var_data =
      var_data_get_type_locked(var, PP_VARTYPE_DICTIONARY);
  *out_var_data = var_data;
  if (var_data == NULL) {
    return PP_FALSE;
  }

  if (!nb_var_check_type_with_error(key, PP_VARTYPE_STRING)) {
    return PP_FALSE;
  }

  for (i = 0; i < var_data->dict.len; ++i) {
    if (!var_string_equal_locked(var_data->dict.keys[i], key)) {
      continue;
    }

    // Found key.
    *out_index = i;

#if FAKE_INTERFACE_TRACE > 1
    VAR_DEBUG_STRING(var, 256);
    VAR_DEBUG_STRING(key, 256);
    NB_VERROR("dict_find(%s, %s) => <%s: %d>", var_str, key_str,
              nb_var_type_to_string(var_data->type), i);
#endif

    return PP_TRUE;
  }

#if FAKE_INTERFACE_TRACE > 1
  VAR_DEBUG_STRING(var, 256);
  VAR_DEBUG_STRING(key, 256);
  NB_VERROR("dict_find(%s, %s) => NOT FOUND", var_str, key_str);
#endif

  // Didn't find key.
  return PP_FALSE;
}

struct PP_Var dict_get(struct PP_Var var, struct PP_Var key) {
  FAKE_INTERFACE_LOCK;
  struct VarData* var_data;
  uint32_t i;
  struct PP_Var result;

  if (!dict_find_locked(var, key, &var_data, &i)) {
#if FAKE_INTERFACE_TRACE > 0
    VAR_DEBUG_STRING(var, 256);
    VAR_DEBUG_STRING(key, 256);
    NB_VERROR("dict_get(%s, %s) => undefined", var_str, key_str);
#endif

    FAKE_INTERFACE_UNLOCK;
    return PP_MakeUndefined();
  }

  result = var_data->dict.values[i];
  var_add_ref_locked(result);

#if FAKE_INTERFACE_TRACE > 0
  VAR_DEBUG_STRING(var, 256);
  VAR_DEBUG_STRING(key, 256);
  VAR_DEBUG_STRING(result, 256);
  NB_VERROR("dict_get(%s, %s) => %s", var_str, key_str, result_str);
#endif

  FAKE_INTERFACE_UNLOCK;
  return result;
}

PP_Bool dict_set(struct PP_Var var, struct PP_Var key, struct PP_Var value) {
  FAKE_INTERFACE_LOCK;
  struct VarData* var_data;
  uint32_t i;
  if (!dict_find_locked(var, key, &var_data, &i)) {
    if (var_data == NULL) {
#if FAKE_INTERFACE_TRACE > 0
      VAR_DEBUG_STRING(var, 256);
      VAR_DEBUG_STRING(key, 256);
      VAR_DEBUG_STRING(value, 256);
      NB_VERROR("dict_set(%s, %s, %s) FAILED", var_str, key_str, value_str);
#endif

      FAKE_INTERFACE_UNLOCK;
      return PP_FALSE;
    }

    if (var_data->dict.len == var_data->dict.cap) {
      uint32_t new_cap = var_data->dict.cap * 2;
      size_t new_size = new_cap * sizeof(struct PP_Var);
      var_data->dict.keys = realloc(var_data->dict.keys, new_size);
      var_data->dict.values = realloc(var_data->dict.values, new_size);
      assert(var_data->dict.keys != NULL);
      assert(var_data->dict.values != NULL);
    }

    i = var_data->dict.len;

    var_add_ref_locked(value);
    var_add_ref_locked(key);
    var_data->dict.keys[i] = key;
    var_data->dict.values[i] = value;
    var_data->dict.len++;

#if FAKE_INTERFACE_TRACE > 0
    VAR_DEBUG_STRING(var, 256);
    VAR_DEBUG_STRING(key, 256);
    VAR_DEBUG_STRING(value, 256);
    NB_VERROR("dict_set(%s, %s, %s)", var_str, key_str, value_str);
#endif

    FAKE_INTERFACE_UNLOCK;
    return PP_TRUE;
  }

  var_add_ref_locked(value);
  var_release_locked(var_data->dict.values[i]);
  var_data->dict.values[i] = value;

#if FAKE_INTERFACE_TRACE > 0
  VAR_DEBUG_STRING(var, 256);
  VAR_DEBUG_STRING(key, 256);
  VAR_DEBUG_STRING(value, 256);
  NB_VERROR("dict_set(%s, %s, %s)", var_str, key_str, value_str);
#endif

  FAKE_INTERFACE_UNLOCK;
  return PP_TRUE;
}

void dict_delete(struct PP_Var var, struct PP_Var key) {
  FAKE_INTERFACE_LOCK;
  struct VarData* var_data;
  uint32_t i;
  size_t size_to_move;
  if (!dict_find_locked(var, key, &var_data, &i)) {
#if FAKE_INTERFACE_TRACE > 0
    VAR_DEBUG_STRING(var, 256);
    VAR_DEBUG_STRING(key, 256);
    NB_VERROR("dict_delete(%s, %s) FAILED", var_str, key_str);
#endif

    FAKE_INTERFACE_UNLOCK;
    return;
  }

  // Found key.
  var_release_locked(var_data->dict.keys[i]);
  var_release_locked(var_data->dict.values[i]);

  // Move everything else down.
  size_to_move = (var_data->dict.len - i - 1) * sizeof(struct PP_Var);
  memmove(&var_data->dict.keys[i], &var_data->dict.keys[i + 1], size_to_move);
  memmove(&var_data->dict.values[i], &var_data->dict.values[i + 1],
          size_to_move);

  var_data->dict.len--;

#if FAKE_INTERFACE_TRACE > 0
  VAR_DEBUG_STRING(var, 256);
  VAR_DEBUG_STRING(key, 256);
  NB_VERROR("dict_delete(%s, %s)", var_str, key_str);
#endif

  FAKE_INTERFACE_UNLOCK;
}

PP_Bool dict_has_key(struct PP_Var var, struct PP_Var key) {
  FAKE_INTERFACE_LOCK;
  struct VarData* var_data;
  uint32_t i;
  PP_Bool result = dict_find_locked(var, key, &var_data, &i);

#if FAKE_INTERFACE_TRACE > 0
  VAR_DEBUG_STRING(var, 256);
  VAR_DEBUG_STRING(key, 256);
  NB_VERROR("dict_has_key(%s, %s) => %s", var_str, key_str,
            result ? "true" : "false");
#endif

  FAKE_INTERFACE_UNLOCK;
  return result;
}

struct PP_Var dict_get_keys(struct PP_Var var) {
  FAKE_INTERFACE_LOCK;
  struct VarData* var_data;
  int i;
  struct PP_Var keys;

  var_data = var_data_get_type_locked(var, PP_VARTYPE_DICTIONARY);
  if (var_data == NULL) {
#if FAKE_INTERFACE_TRACE > 0
    VAR_DEBUG_STRING(var, 256);
    NB_VERROR("dict_get_keys(%s) FAILED", var_str);
#endif

    FAKE_INTERFACE_UNLOCK;
    return PP_MakeUndefined();
  }

  keys = array_create_locked();
  for (i = 0; i < var_data->dict.len; ++i) {
    if (!array_set_locked(keys, i, var_data->dict.keys[i])) {
      var_release_locked(keys);
      FAKE_INTERFACE_UNLOCK;
      return PP_MakeUndefined();
    }
  }

#if FAKE_INTERFACE_TRACE > 0
  VAR_DEBUG_STRING(var, 256);
  VAR_DEBUG_STRING(keys, 256);
  NB_VERROR("dict_get_keys(%s) => %s", var_str, keys_str);
#endif

  FAKE_INTERFACE_UNLOCK;
  return keys;
}

void messaging_post_message(PP_Instance instance, struct PP_Var message) {
  FAKE_INTERFACE_LOCK;
  if (!s_post_message_callback) {
    NB_ERROR("PostMessage called without a fake post message callback.");
    FAKE_INTERFACE_UNLOCK;
    return;
  }

#if FAKE_INTERFACE_TRACE > 0
  VAR_DEBUG_STRING(message, 256);
  NB_VERROR("messagingpost_message(%s)", message_str);
#endif

  FAKE_INTERFACE_UNLOCK;

  /* Don't hold the interface lock when calling the callback. */
  (*s_post_message_callback)(message, s_post_message_callback_user_data);
}
