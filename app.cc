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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <zlib.h>

#include <map>
#include <string>
#include <vector>

#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_array.h>
#include <ppapi/cpp/var_dictionary.h>
#include <ppapi/utility/completion_callback_factory.h>

#ifdef WIN32
#undef PostMessage
// Allow 'this' in initializer list
#pragma warning(disable : 4355)
#endif

#define VAR_DICTIONARY(var, newvar) \
  if (!var.is_dictionary()) { \
    printf(#var " is not a dictionary.\n"); \
    return; \
  } \
  pp::VarDictionary newvar(var)

#define DICT_INT(var, key) \
  int32_t key; \
  do { \
    pp::Var tempvar(var.Get(#key)); \
    if (!tempvar.is_int()) { \
      printf(#key " is not an int.\n"); \
      return; \
    } \
    key = tempvar.AsInt(); \
  } while(0)

#define DICT_STRING(var, key) \
  std::string key; \
  do { \
    pp::Var tempvar(var.Get(#key)); \
    if (!tempvar.is_string()) { \
      printf(#key " is not an string.\n"); \
      return; \
    } \
    key = tempvar.AsString(); \
  } while(0)

#define DICT_ARRAY(var, key) \
  pp::VarArray key; \
  do { \
    pp::Var tempvar(var.Get(#key)); \
    if (!tempvar.is_array()) { \
      printf(#key " is not an array.\n"); \
      return; \
    } \
    key = pp::VarArray(tempvar); \
  } while(0)

#define ARRAY_INT(var, ix, newvar) \
  int32_t newvar; \
  do { \
    pp::Var tempvar(var.Get(ix)); \
    if (!tempvar.is_int()) { \
      printf("Argument %d is not an int.\n", ix); \
      return; \
    } \
    newvar = tempvar.AsInt(); \
  } while(0)

#define ARRAY_HANDLE(var, ix, newvar) \
  void* newvar; \
  do { \
    ARRAY_INT(var, ix, newvar##int); \
    newvar = GetHandle(newvar##int); \
    if (!newvar) { \
      printf("Argument %d is not an valid handle.\n", ix); \
      return; \
    } \
  } while(0)

#define ARRAY_ARRAY(var, ix, newvar) \
  pp::VarArray newvar; \
  do { \
    pp::Var tempvar(var.Get(ix)); \
    if (!tempvar.is_array()) { \
      printf("Argument %d is not an array.\n", ix); \
      return; \
    } \
    newvar = pp::VarArray(tempvar); \
  } while(0)

#define ARRAY_ARRAYBUFFER(var, ix, newvar) \
  pp::VarArrayBuffer newvar; \
  do { \
    pp::Var tempvar(var.Get(ix)); \
    if (!tempvar.is_array_buffer()) { \
      printf("Argument %d is not an array buffer.\n", ix); \
      return; \
    } \
    newvar = pp::VarArrayBuffer(tempvar); \
  } while(0)

typedef int32_t TypeId;
class Type;

typedef std::map<TypeId, Type*> TypeMap;
TypeMap g_type_map;

class Type {
 public:
  explicit Type(TypeId id) : id_(id) {}
  virtual size_t Size() = 0;
  virtual std::string ToString() = 0;
  virtual bool IsPrimitiveType() { return false; }

  TypeId id() { return id_; }

 private:
  TypeId id_;
};

class PrimitiveType : public Type {
 public:
  PrimitiveType(TypeId id, const char* name, size_t size)
      : Type(id), name_(name), size_(size) {}

  virtual size_t Size() { return size_; }
  virtual std::string ToString() { return name_; }
  virtual bool IsPrimitiveType() { return true; }

 private:
  const char* name_;
  size_t size_;
};

class PointerType : public Type {
 public:
  PointerType(TypeId id, Type* base_type) : Type(id), base_type_(base_type) {}
  virtual size_t Size() { return 4; }
  virtual std::string ToString() { return base_type_->ToString() + "*"; }

 private:
  Type* base_type_;
};

struct StructField {
  const char* name;
  Type* type;
  size_t offset;
};

class StructType : public Type {
 public:
  StructType(TypeId id, const char* name, size_t size, int num_fields,
             StructField* fields)
      : Type(id), name_(name), size_(size) {
    for (int i = 0; i < num_fields; ++i) {
      fields_.push_back(fields[i]);
    }
  }
  virtual size_t Size() { return size_; }
  virtual std::string ToString() { return name_; }

 private:
  const char* name_;
  size_t size_;
  std::vector<StructField> fields_;
};

class FunctionType : public Type {
 public:
  FunctionType(TypeId id, Type* ret_type, Type** arg_types)
      : Type(id), ret_type_(ret_type) {
    while (arg_types && *arg_types) {
      arg_types_.push_back(*arg_types);
      arg_types++;
    }
  }
  virtual size_t Size() { return 4; }
  virtual std::string ToString() {
    std::string result;
    result += ret_type_->ToString();
    result += " (*)(";
    for (size_t i = 0; i < arg_types_.size(); ++i) {
      result += arg_types_[i]->ToString();
      if (i < arg_types_.size() - 1) {
        result += ", ";
      }
    }
    result += ")";
    return result;
  }

 private:
  Type* ret_type_;
  std::vector<Type*> arg_types_;
};

class PepperType {
 public:
  PepperType(const char* name, PP_VarType var_type)
      : name_(name), var_type_(var_type) {}

  virtual size_t Size() { return sizeof(pp::Var); }
  virtual std::string ToString() { return name_; }

 private:
  const char* name_;
  PP_VarType var_type_;
};

PrimitiveType TYPE_void(1, "void", 0);
PointerType TYPE_void_p(2, &TYPE_void);
PrimitiveType TYPE_int8(3, "int8", 1);
PrimitiveType TYPE_uint8(4, "uint8", 1);
PrimitiveType TYPE_int16(5, "int16", 2);
PrimitiveType TYPE_uint16(6, "uint16", 2);
PrimitiveType TYPE_int32(7, "int32", 4);
PrimitiveType TYPE_uint32(8, "uint32", 4);
PrimitiveType TYPE_int64(9, "int64", 8);
PrimitiveType TYPE_uint64(10, "uint64", 8);
PrimitiveType TYPE_float32(11, "float32", 4);
PrimitiveType TYPE_float64(12, "float64", 8);
PrimitiveType TYPE_size_t(13, "size_t", 4);
PointerType TYPE_uint8_p(14, &TYPE_uint8);
PointerType TYPE_uint8_pp(15, &TYPE_uint8_p);
PointerType TYPE_uint32_p(16, &TYPE_uint8_p);
PepperType TYPE_arrayBuffer(17, PP_VARTYPE_ARRAY_BUFFER);
PepperType TYPE_array(18, PP_VARTYPE_ARRAY);
PepperType TYPE_dictionary(19, PP_VARTYPE_DICTIONARY);

Type* TYPE_malloc_args[] = { &TYPE_size_t, NULL };
FunctionType TYPE_malloc(20, &TYPE_void_p, TYPE_malloc_args);

Type* TYPE_memset_args[] = { &TYPE_void_p, &TYPE_int32, &TYPE_size_t, NULL };
FunctionType TYPE_memset(21, &TYPE_void, TYPE_memset_args);

Type* TYPE_memcpy_args[] = { &TYPE_void_p, &TYPE_void_p, &TYPE_size_t, NULL };
FunctionType TYPE_memcpy(22, &TYPE_void, TYPE_memcpy_args);

Type* TYPE_mapArrayBuffer_args = { &TYPE_arrayBuffer, NULL };
FunctionType TYPE_mapArrayBuffer(23, &TYPE_void_p, TYPE_mapArrayBuffer_args);

Type* TYPE_add_void_p_int32 = { &TYPE_void_p, &TYPE_int32, NULL };
FunctionType TYPE_add_void_p_int32(24, &TYPE_void_p, &TYPE_add_void_p_int32);

StructField TYPE_z_stream_fields[] = {
  {"next_in", &TYPE_uint8_p, offsetof(z_stream, next_in)},
  {"avail_in", &TYPE_uint32, offsetof(z_stream, avail_in)},
  {"total_in", &TYPE_uint32, offsetof(z_stream, total_in)},
  {"next_out", &TYPE_uint8_p, offsetof(z_stream, next_out)},
  {"avail_out", &TYPE_uint32, offsetof(z_stream, avail_out)},
  {"total_out", &TYPE_uint32, offsetof(z_stream, total_out)},
};
StructType TYPE_z_stream(
    18, "z_stream", sizeof(z_stream),
    sizeof(TYPE_z_stream_fields)/sizeof(TYPE_z_stream_fields[0]),
    TYPE_z_stream_fields);
PointerType TYPE_z_stream_p(25, &TYPE_z_stream);

Type* TYPE_deflate_args[] = { &TYPE_int32, &TYPE_z_stream_p, &TYPE_int32,
                              NULL };
FunctionType TYPE_deflate(26, &TYPE_void, TYPE_deflate_args);


class Instance : public pp::Instance {
 public:
  explicit Instance(PP_Instance instance)
      : pp::Instance(instance),
        callback_factory_(this) {}

  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
    return true;
  }

  virtual void HandleMessage(const pp::Var& var) {
    VAR_DICTIONARY(var, dictionary);
    DICT_INT(dictionary, id);
    DICT_ARRAY(dictionary, msgs);
    DICT_ARRAY(dictionary, handles);

    for (uint32_t i = 0; i < msgs.GetLength(); ++i) {
      HandleCommand(msgs.Get(i));
    }

    pp::VarArray values;
    values.SetLength(handles.GetLength());

    // Handles to return to the JavaScript.
    for (uint32_t i = 0; i < handles.GetLength(); ++i) {
      ARRAY_ARRAY(handles, i, handle);
      ARRAY_HANDLE(handle, 0, ptr);
      ARRAY_INT(handle, 0, id);
      ARRAY_INT(handle, 1, type_id);

      bool is_primitive = true;

      if (type_id == TYPE_int8.id()) {
        values.Set(i, *(int8_t*)ptr);
      } else if (type_id == TYPE_uint8.id()) {
        values.Set(i, *(uint8_t*)ptr);
      } else if (type_id == TYPE_int16.id()) {
        values.Set(i, *(int16_t*)ptr);
      } else if (type_id == TYPE_uint16.id()) {
        values.Set(i, *(uint16_t*)ptr);
      } else if (type_id == TYPE_int32.id()) {
        values.Set(i, *(int32_t*)ptr);
      } else if (type_id == TYPE_uint32.id()) {
        values.Set(i, (int32_t)*(uint32_t*)ptr);
//       } else if (type_id == TYPE_int64.id()) {
//         values.Set(i, *(int64_t*)ptr);
//       } else if (type_id == TYPE_uint64.id()) {
//         values.Set(i, *(uint64_t*)ptr);
      } else if (type_id == TYPE_float32.id()) {
        values.Set(i, *(float*)ptr);
      } else if (type_id == TYPE_float64.id()) {
        values.Set(i, *(double*)ptr);
      //} else if (type_id == TYPE_size_t.id()) {
      //  values.Set(i, *(size_t*)ptr);
      } else {
        is_primitive = false;
        // Not a primitive value, send the id.
        values.Set(i, id);
      }

      if (is_primitive) {
        DestroyHandle(id);
      }
    }

    pp::VarDictionary result;
    result.Set("id", id);
    result.Set("values", values);
    PostMessage(result);
  }

 private:
  typedef int32_t Handle;
  typedef std::map<Handle, void*> HandleMap;
  HandleMap handle_map_;

  void* GetHandle(Handle handle) {
    HandleMap::iterator iter = handle_map_.find(handle);
    if (iter == handle_map_.end())
      return NULL;
    return iter->second;
  }

  void RegisterHandle(Handle handle, void* pointer) {
    HandleMap::iterator iter = handle_map_.find(handle);
    if (iter != handle_map_.end()) {
      printf("RegisterHandle: handle %d is already registered.\n", handle);
      return;
    }

    handle_map_.insert(HandleMap::value_type(handle, pointer));
  }

  void DestroyHandle(Handle handle) {
    HandleMap::iterator iter = handle_map_.find(handle);
    if (iter == handle_map_.end()) {
      printf("DestroyHandle: handle %d doesn't exist.\n", handle);
      return;
    }

    handle_map_.erase(iter);
  }

  void HandleCommand(const pp::Var& msg) {
    VAR_DICTIONARY(msg, dictionary);
    DICT_STRING(dictionary, cmd);
    DICT_INT(dictionary, type);
    DICT_ARRAY(dictionary, args);
    DICT_INT(dictionary, ret);

    if (cmd == "add") {
    } else if (cmd == "addr") {
    } else if (cmd == "malloc") {
      CHECK(type == TYPE_malloc);
      Handle_malloc(ret, args);
    } else if (cmd == "memset") {
      CHECK(type == TYPE_memset);
      Handle_memset(ret, args);
    } else if (cmd == "memcpy") {
      CHECK(type == TYPE_memcpy);
      Handle_memcpy(ret, args);
    } else if (cmd == "mapArrayBuffer") {
      CHECK(type == TYPE_mapArrayBuffer);
      Handle_mapArrayBuffer(ret, args);
    } else if (cmd == "deflateInit") {
      CHECK(type == TYPE_deflate);
      Handle_deflateInit(ret, args);
    } else {
      printf("Unknown cmd: \"%s\".\n", cmd.c_str());
    }
  }

  void Handle_malloc(Handle ret_handle, const pp::VarArray& args) {
    ARRAY_INT(args, 0, size);
    void* result = malloc(size);
    RegisterHandle(ret_handle, result);

    printf("malloc(%d) => %p (%d)\n", size, result, ret_handle);
  }

  void Handle_memset(Handle ret_handle, const pp::VarArray& args) {
    ARRAY_HANDLE(args, 0, buffer);
    ARRAY_INT(args, 1, value);
    ARRAY_INT(args, 2, size);
    memset(buffer, value, size);
    printf("memset(%p, %d, %d)\n", buffer, value, size);
  }

  void Handle_memcpy(Handle ret_handle, const pp::VarArray& args) {
    ARRAY_HANDLE(args, 0, dst);
    ARRAY_HANDLE(args, 1, src);
    ARRAY_INT(args, 2, size);
    printf("memcpy(%p, %p, %d)\n", dst, src, size);
    memcpy(dst, src, size);
  }

  void Handle_mapArrayBuffer(Handle ret_handle, const pp::VarArray& args) {
    ARRAY_ARRAYBUFFER(args, 0, buf);
    void* ptr = buf.Map();
    printf("mapArrayBuffer(%lld) => %p\n", buf.pp_var().value.as_id, ptr);
    RegisterHandle(ret_handle, ptr);
  }

  void Handle_deflateInit(Handle ret_handle, const pp::VarArray& args) {
    ARRAY_HANDLE(args, 0, stream);
    ARRAY_INT(args, 1, level);
    int result = deflateInit((z_stream*)stream, level);
    printf("deflateInit(%p, %d) => %d\n", stream, level, result);

    // TODO(binji): putting the result in allocated memory kinda sucks.
    // Something better here?
    RegisterHandle(ret_handle, new int32_t(result));
  }

 private:
  pp::CompletionCallbackFactory<Instance> callback_factory_;
};

class Module : public pp::Module {
 public:
  Module() : pp::Module() {}
  virtual ~Module() {}

  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new Instance(instance);
  }
};

namespace pp {
Module* CreateModule() { return new ::Module(); }
}  // namespace pp
