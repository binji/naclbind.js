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
#include <limits>
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

#define ERROR(msg, ...) \
  fprintf(stderr, "%s:%d: " msg "\n", __FILE__, __LINE__, __VA_ARGS__)

void VarTypeError(const char* file, int line, const char* var_name,
                  const char* expected_type) {
  fprintf(stderr, "%s:%d: %s is not of type %s.\n", file, line, var_name,
          expected_type);
}

#define CHECK_VARTYPE(var, var_name, type)             \
  if (!var.is_##type()) {                              \
    VarTypeError(__FILE__, __LINE__, var_name, #type); \
    return;                                            \
  }

#define VAR_DICTIONARY(var, newvar)     \
  CHECK_VARTYPE(var, #var, dictionary); \
  pp::VarDictionary newvar(var)

#define DICT_INT(var, key)             \
  int32_t key;                         \
  do {                                 \
    pp::Var tempvar(var.Get(#key));    \
    CHECK_VARTYPE(tempvar, #key, int); \
    key = tempvar.AsInt();             \
  } while (0)

#define DICT_STRING(var, key)             \
  std::string key;                        \
  do {                                    \
    pp::Var tempvar(var.Get(#key));       \
    CHECK_VARTYPE(tempvar, #key, string); \
    key = tempvar.AsString();             \
  } while (0)

#define DICT_ARRAY(var, key)             \
  pp::VarArray key;                      \
  do {                                   \
    pp::Var tempvar(var.Get(#key));      \
    CHECK_VARTYPE(tempvar, #key, array); \
    key = pp::VarArray(tempvar);         \
  } while (0)

#define ARRAY_INT(var, ix, newvar)                \
  int32_t newvar;                                 \
  do {                                            \
    pp::Var tempvar(var.Get(ix));                 \
    CHECK_VARTYPE(tempvar, "Argument " #ix, int); \
    newvar = tempvar.AsInt();                     \
  } while (0)

#define ARRAY_HANDLE(var, ix, newvar)                  \
  void* newvar;                                        \
  do {                                                 \
    ARRAY_INT(var, ix, newvar##int);                   \
    HandleObject hobj;                                 \
    if (!GetHandle(newvar##int, &hobj)) {              \
      ERROR("Argument %d is not a valid handle.", ix); \
      return;                                          \
    }                                                  \
    newvar = hobj.ptr;                                 \
  } while (0)

#define ARRAY_ARRAY(var, ix, newvar)                \
  pp::VarArray newvar;                              \
  do {                                              \
    pp::Var tempvar(var.Get(ix));                   \
    CHECK_VARTYPE(tempvar, "Argument " #ix, array); \
    newvar = pp::VarArray(tempvar);                 \
  } while (0)

#define ARRAY_ARRAYBUFFER(var, ix, newvar)                 \
  pp::VarArrayBuffer newvar;                               \
  do {                                                     \
    pp::Var tempvar(var.Get(ix));                          \
    CHECK_VARTYPE(tempvar, "Argument " #ix, array_buffer); \
    newvar = pp::VarArrayBuffer(tempvar);                  \
  } while (0)

typedef int32_t TypeId;
class Type;

typedef std::map<TypeId, Type*> TypeMap;
TypeMap g_type_map;

class Type {
 public:
  explicit Type(TypeId id) : id_(id) {
    TypeMap::iterator iter = g_type_map.find(id);
    if (iter != g_type_map.end()) {
      ERROR("Type id %d already used!\n", id);
      return;
    }

    g_type_map.insert(TypeMap::value_type(id, this));
  }
  virtual ~Type() {}
  virtual size_t Size() = 0;
  virtual std::string ToString() = 0;
  virtual bool IsPrimitiveType() { return false; }

  TypeId id() { return id_; }

  static Type* Get(TypeId id) {
    TypeMap::iterator iter = g_type_map.find(id);
    if (iter == g_type_map.end()) {
      return NULL;
    }

    return iter->second;
  }

  template <typename T>
  static Type* Get() { return NULL; }

  virtual bool GetValue(void* ptr, int8_t* out_value) { return false; }
  virtual bool GetValue(void* ptr, uint8_t* out_value) { return false; }
  virtual bool GetValue(void* ptr, int16_t* out_value) { return false; }
  virtual bool GetValue(void* ptr, uint16_t* out_value) { return false; }
  virtual bool GetValue(void* ptr, int32_t* out_value) { return false; }
  virtual bool GetValue(void* ptr, uint32_t* out_value) { return false; }
  virtual bool GetValue(void* ptr, int64_t* out_value) { return false; }
  virtual bool GetValue(void* ptr, uint64_t* out_value) { return false; }
  virtual bool GetValue(void* ptr, float* out_value) { return false; }
  virtual bool GetValue(void* ptr, double* out_value) { return false; }
  virtual bool GetValue(void* ptr, void** out_value) { return false; }
  virtual bool GetValue(void* ptr, pp::VarArrayBuffer* out_value) {
    return false;
  }

  template <typename T>
  bool GetValue(void* ptr, T* out_value);

  template <typename T>
  bool GetValue(void* ptr, T** out_value) {
    void* void_ptr;
    if (!GetValue(ptr, &void_ptr)) {
      ERROR("Can't get void pointer. %d", 0);
      return false;
    }

    *out_value = static_cast<T*>(void_ptr);
    return true;
  }

 private:
  TypeId id_;
};

class VoidType : public Type {
 public:
  explicit VoidType(TypeId id) : Type(id) {}

  virtual size_t Size() { return 0; }
  virtual std::string ToString() { return "void"; }
  virtual bool IsPrimitiveType() { return false; }
};

template <typename T>
class PrimitiveType : public Type {
 public:
  PrimitiveType(TypeId id, const char* name)
      : Type(id), name_(name) {}

  virtual size_t Size() { return sizeof(T); }
  virtual std::string ToString() { return name_; }
  virtual bool IsPrimitiveType() { return true; }

  virtual bool GetValue(void* ptr, int8_t* out_value) {
    return GetValue<int8_t>(ptr, out_value);
  }

  virtual bool GetValue(void* ptr, uint8_t* out_value) {
    return GetValue<uint8_t>(ptr, out_value);
  }

  virtual bool GetValue(void* ptr, int16_t* out_value) {
    return GetValue<int16_t>(ptr, out_value);
  }

  virtual bool GetValue(void* ptr, uint16_t* out_value) {
    return GetValue<uint16_t>(ptr, out_value);
  }

  virtual bool GetValue(void* ptr, int32_t* out_value) {
    return GetValue<int32_t>(ptr, out_value);
  }

  virtual bool GetValue(void* ptr, uint32_t* out_value) {
    return GetValue<uint32_t>(ptr, out_value);
  }

  virtual bool GetValue(void* ptr, int64_t* out_value) {
    return GetValue<int64_t>(ptr, out_value);
  }

  virtual bool GetValue(void* ptr, uint64_t* out_value) {
    return GetValue<uint64_t>(ptr, out_value);
  }

  virtual bool GetValue(void* ptr, float* out_value) {
    return GetValue<float>(ptr, out_value);
  }

  virtual bool GetValue(void* ptr, double* out_value) {
    return GetValue<double>(ptr, out_value);
  }

 private:
  template <typename U>
  bool GetValue(void* ptr, U* out_value) {
    *out_value = *static_cast<T*>(ptr);
    return true;
  }

  const char* name_;
};

class PointerType : public Type {
 public:
  PointerType(TypeId id, Type* base_type) : Type(id), base_type_(base_type) {}
  virtual size_t Size() { return 4; }
  virtual std::string ToString() { return base_type_->ToString() + "*"; }

  virtual bool GetValue(void* ptr, void** out_value) {
    *out_value = static_cast<void*>(ptr);
    return true;
  }

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
  FunctionType(TypeId id, Type* ret_type) : Type(id), ret_type_(ret_type) {}

  FunctionType(TypeId id, Type* ret_type, Type* arg0)
      : Type(id), ret_type_(ret_type) {
    arg_types_.push_back(arg0);
  }

  FunctionType(TypeId id, Type* ret_type, Type* arg0, Type* arg1)
      : Type(id), ret_type_(ret_type) {
    arg_types_.push_back(arg0);
    arg_types_.push_back(arg1);
  }

  FunctionType(TypeId id, Type* ret_type, Type* arg0, Type* arg1, Type* arg2)
      : Type(id), ret_type_(ret_type) {
    arg_types_.push_back(arg0);
    arg_types_.push_back(arg1);
    arg_types_.push_back(arg2);
  }

  FunctionType(TypeId id, Type* ret_type, Type* arg0, Type* arg1, Type* arg2,
               Type* arg3)
      : Type(id), ret_type_(ret_type) {
    arg_types_.push_back(arg0);
    arg_types_.push_back(arg1);
    arg_types_.push_back(arg2);
    arg_types_.push_back(arg3);
  }

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

  Type* ret_type() { return ret_type_; }

  Type* GetArgType(size_t ix) {
    if (ix >= arg_types_.size()) {
      return NULL;
    }
    return arg_types_[ix];
  }

 private:
  Type* ret_type_;
  std::vector<Type*> arg_types_;
};

class PepperType : public Type {
 public:
  PepperType(TypeId id, const char* name, PP_VarType var_type)
      : Type(id), name_(name), var_type_(var_type) {}

  virtual size_t Size() { return sizeof(pp::Var); }
  virtual std::string ToString() { return name_; }

  PP_VarType var_type() const { return var_type_; }

  virtual bool GetValue(void* ptr, pp::VarArrayBuffer* out_value) {
    if (var_type_ != PP_VARTYPE_ARRAY_BUFFER) {
      return false;
    }
    pp::Var var(*static_cast<PP_Var*>(ptr));
    *out_value = pp::VarArrayBuffer(var);
    return true;
  }

 private:
  const char* name_;
  PP_VarType var_type_;
};

VoidType TYPE_void(1);
PrimitiveType<int8_t> TYPE_int8(2, "int8");
PrimitiveType<uint8_t> TYPE_uint8(3, "uint8");
PrimitiveType<int16_t> TYPE_int16(4, "int16");
PrimitiveType<uint16_t> TYPE_uint16(5, "uint16");
PrimitiveType<int32_t> TYPE_int32(6, "int32");
PrimitiveType<uint32_t> TYPE_uint32(7, "uint32");
PrimitiveType<int64_t> TYPE_int64(8, "int64");
PrimitiveType<int64_t> TYPE_uint64(9, "uint64");
PrimitiveType<float> TYPE_float32(10, "float32");
PrimitiveType<double> TYPE_float64(11, "float64");
PrimitiveType<size_t> TYPE_size_t(12, "size_t");

PointerType TYPE_void_p(13, &TYPE_void);
PointerType TYPE_uint8_p(14, &TYPE_uint8);
PointerType TYPE_uint8_pp(15, &TYPE_uint8_p);
PointerType TYPE_uint32_p(16, &TYPE_uint32);
PepperType TYPE_arrayBuffer(17, "ArrayBuffer", PP_VARTYPE_ARRAY_BUFFER);
PepperType TYPE_array(18, "Array", PP_VARTYPE_ARRAY);
PepperType TYPE_dictionary(19, "Dictionary", PP_VARTYPE_DICTIONARY);
FunctionType TYPE_malloc(20, &TYPE_void_p, &TYPE_size_t);
FunctionType TYPE_memset(21, &TYPE_void, &TYPE_void_p, &TYPE_int32,
                         &TYPE_size_t);
FunctionType TYPE_memcpy(22, &TYPE_void, &TYPE_void_p, &TYPE_void_p,
                         &TYPE_size_t);
FunctionType TYPE_add_void_p_int32(23, &TYPE_void_p, &TYPE_void_p, &TYPE_int32);
FunctionType TYPE_set_uint8_p(24, &TYPE_void, &TYPE_uint8_pp, &TYPE_uint8_p);
FunctionType TYPE_set_uint32(25, &TYPE_void, &TYPE_uint32_p, &TYPE_uint32);
FunctionType TYPE_get_uint8_p(26, &TYPE_uint8_p, &TYPE_uint8_pp);
FunctionType TYPE_get_uint32(27, &TYPE_uint32, &TYPE_uint32_p);
FunctionType TYPE_sub_int32(28, &TYPE_int32, &TYPE_int32, &TYPE_int32);
FunctionType TYPE_sub_uint32(29, &TYPE_uint32, &TYPE_uint32, &TYPE_uint32);
FunctionType TYPE_arrayBufferCreate(30, &TYPE_arrayBuffer, &TYPE_uint32);
FunctionType TYPE_arrayBufferMap(31, &TYPE_void_p, &TYPE_arrayBuffer);
FunctionType TYPE_arrayBufferUnmap(32, &TYPE_void, &TYPE_arrayBuffer);

StructField TYPE_z_stream_fields[] = {
    {"next_in", &TYPE_uint8_p, offsetof(z_stream, next_in)},
    {"avail_in", &TYPE_uint32, offsetof(z_stream, avail_in)},
    {"total_in", &TYPE_uint32, offsetof(z_stream, total_in)},
    {"next_out", &TYPE_uint8_p, offsetof(z_stream, next_out)},
    {"avail_out", &TYPE_uint32, offsetof(z_stream, avail_out)},
    {"total_out", &TYPE_uint32, offsetof(z_stream, total_out)}, };
StructType TYPE_z_stream(33, "z_stream", sizeof(z_stream),
                         sizeof(TYPE_z_stream_fields) /
                             sizeof(TYPE_z_stream_fields[0]),
                         TYPE_z_stream_fields);
PointerType TYPE_z_stream_p(34, &TYPE_z_stream);

FunctionType TYPE_deflate(35, &TYPE_int32, &TYPE_z_stream_p, &TYPE_int32);


template <> Type* Type::Get<int8_t>() { return &TYPE_int8; }
template <> Type* Type::Get<uint8_t>() { return &TYPE_uint8; }
template <> Type* Type::Get<int16_t>() { return &TYPE_int16; }
template <> Type* Type::Get<uint16_t>() { return &TYPE_uint16; }
template <> Type* Type::Get<int32_t>() { return &TYPE_int32; }
template <> Type* Type::Get<uint32_t>() { return &TYPE_uint32; }
template <> Type* Type::Get<int64_t>() { return &TYPE_int64; }
template <> Type* Type::Get<uint64_t>() { return &TYPE_uint64; }
template <> Type* Type::Get<float>() { return &TYPE_float32; }
template <> Type* Type::Get<double>() { return &TYPE_float64; }
template <> Type* Type::Get<void*>() { return &TYPE_void_p; }


typedef int32_t Handle;

struct HandleObject {
  HandleObject() : ptr(NULL), type(NULL) {}
  HandleObject(void* ptr, Type* type) : ptr(ptr), type(type) {}

  void* ptr;
  Type* type;
};

typedef std::map<Handle, HandleObject> HandleMap;
HandleMap g_handle_map;

bool GetHandle(Handle handle, HandleObject* out_hobj) {
  HandleMap::iterator iter = g_handle_map.find(handle);
  if (iter == g_handle_map.end()) {
    return false;
  }

  *out_hobj = iter->second;
  return true;
}

template <typename T>
bool GetHandleValue(Handle handle, T* out_value) {
  HandleObject hobj;
  if (!GetHandle(handle, &hobj)) {
    ERROR("GetHandleValue: Invalid handle %d.", handle);
    return false;
  }

  Type* src_type = hobj.type;
  return src_type->GetValue(hobj.ptr, out_value);
}

void RegisterHandle(Handle handle, void* pointer, Type* type) {
  HandleMap::iterator iter = g_handle_map.find(handle);
  if (iter != g_handle_map.end()) {
    ERROR("RegisterHandle: handle %d is already registered.\n", handle);
    return;
  }

  g_handle_map.insert(
      HandleMap::value_type(handle, HandleObject(pointer, type)));
}

template <typename T>
void RegisterHandle(Handle handle, T arg) {
  // TODO(binji): putting the result in allocated memory kinda sucks.
  // Something better here?
  RegisterHandle(handle, new T(arg), Type::Get<T>());
}

template <typename T>
void RegisterHandle(Handle handle, T* pointer) {
  RegisterHandle(handle, pointer, Type::Get<T*>());
}

template <>
void RegisterHandle(Handle handle, pp::VarArrayBuffer* array_buffer) {
  PP_Var var = array_buffer->Detach();
  RegisterHandle(handle, new PP_Var(var), &TYPE_arrayBuffer);
}

void DestroyHandle(Handle handle) {
  HandleMap::iterator iter = g_handle_map.find(handle);
  if (iter == g_handle_map.end()) {
    ERROR("DestroyHandle: handle %d doesn't exist.\n", handle);
    return;
  }

  g_handle_map.erase(iter);
}


template <typename T>
bool GetVarValue(const pp::Var& var, T* out_value) {
  return false;
}

template <typename T>
bool GetVarIntegerValue(const pp::Var& var, T* out_value) {
  if (!var.is_int()) {
    return false;
  }

  *out_value = var.AsInt();
  return true;
}

template <>
bool GetVarValue(const pp::Var& var, int8_t* out_value) {
  return GetVarIntegerValue(var, out_value);
}

template <>
bool GetVarValue(const pp::Var& var, uint8_t* out_value) {
  return GetVarIntegerValue(var, out_value);
}

template <>
bool GetVarValue(const pp::Var& var, int16_t* out_value) {
  return GetVarIntegerValue(var, out_value);
}

template <>
bool GetVarValue(const pp::Var& var, uint16_t* out_value) {
  return GetVarIntegerValue(var, out_value);
}

template <>
bool GetVarValue(const pp::Var& var, int32_t* out_value) {
  return GetVarIntegerValue(var, out_value);
}

template <>
bool GetVarValue(const pp::Var& var, uint32_t* out_value) {
  return GetVarIntegerValue(var, out_value);
}

template <typename T>
bool GetVarFloatValue(const pp::Var& var, T* out_value) {
  if (!var.is_double()) {
    return false;
  }

  *out_value = var.AsDouble();
  return true;
}

template <>
bool GetVarValue(const pp::Var& var, float* out_value) {
  return GetVarFloatValue(var, out_value);
}

template <>
bool GetVarValue(const pp::Var& var, double* out_value) {
  return GetVarFloatValue(var, out_value);
}

template <>
bool GetVarValue(const pp::Var& var, pp::VarArrayBuffer* out_value) {
  if (!var.is_array_buffer()) {
    return false;
  }

  *out_value = pp::VarArrayBuffer(var);
  return true;
}


class CommandProcessor {
 public:
  CommandProcessor(FunctionType* func_type, const pp::VarArray& args,
                   const pp::VarArray& arg_is_handle)
      : func_type_(func_type),
        args_(args),
        arg_is_handle_(arg_is_handle) {}

  FunctionType* func_type() const { return func_type_; }

  bool IsTypeValid(FunctionType* expected) const {
    return func_type_ == expected;
  }

  template <typename T>
  bool Get(int index, T* out_value) const {
    pp::Var arg = args_.Get(index);
    if (IsHandle(index)) {
      if (!arg.is_int()) {
        ERROR("Expected Handle, but arg %d is not int.", index);
        return false;
      }

      Handle handle = arg.AsInt();
      return GetHandleValue<T>(handle, out_value);
    } else {
      return GetVarValue<T>(arg, out_value);
    }
  }

  bool IsHandle(int index) const {
    pp::Var var = arg_is_handle_.Get(index);
    return var.is_bool() ? var.AsBool() : false;
  }

 private:
  FunctionType* func_type_;
  pp::VarArray args_;
  pp::VarArray arg_is_handle_;
};


class Instance : public pp::Instance {
 public:
  explicit Instance(PP_Instance instance)
      : pp::Instance(instance), callback_factory_(this) {}

  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
    return true;
  }

  virtual void HandleMessage(const pp::Var& var) {
    VAR_DICTIONARY(var, dictionary);
    DICT_INT(dictionary, id);
    DICT_ARRAY(dictionary, msgs);
    DICT_ARRAY(dictionary, handles);

    printf("Commands:\n");
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

      bool unwrapped = true;

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
        values.Set(i, (int32_t) * (uint32_t*)ptr);
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
      } else if (type_id == TYPE_arrayBuffer.id()) {
        values.Set(i, pp::Var(pp::PASS_REF, *((PP_Var*)ptr)));
      } else {
        unwrapped = false;
        // Not a primitive value, send the id.
        values.Set(i, id);
      }

      if (unwrapped) {
        DestroyHandle(id);
      }
    }

    pp::VarDictionary result;
    result.Set("id", id);
    result.Set("values", values);
    PostMessage(result);
  }

 private:
  void HandleCommand(const pp::Var& msg) {
    VAR_DICTIONARY(msg, dictionary);
    DICT_STRING(dictionary, cmd);
    DICT_INT(dictionary, type);
    DICT_ARRAY(dictionary, args);
    DICT_ARRAY(dictionary, argIsHandle);
    DICT_INT(dictionary, ret);
    FunctionType* func_type = static_cast<FunctionType*>(Type::Get(type));

    CommandProcessor cmdproc(func_type, args, argIsHandle);

    if (cmd == "add") {
      Handle_add(cmdproc, ret);
    } else if (cmd == "arrayBufferCreate") {
      Handle_arrayBufferCreate(cmdproc, ret);
    } else if (cmd == "arrayBufferMap") {
      Handle_arrayBufferMap(cmdproc, ret);
    } else if (cmd == "deflate") {
      Handle_deflate(cmdproc, ret);
    } else if (cmd == "deflateInit") {
      Handle_deflateInit(cmdproc, ret);
    } else if (cmd == "get") {
      Handle_get(cmdproc, ret);
    } else if (cmd == "malloc") {
      Handle_malloc(cmdproc, ret);
    } else if (cmd == "memcpy") {
      Handle_memcpy(cmdproc, ret);
    } else if (cmd == "memset") {
      Handle_memset(cmdproc, ret);
    } else if (cmd == "set") {
      Handle_set(cmdproc, ret);
    } else if (cmd == "sub") {
      Handle_sub(cmdproc, ret);
    } else {
      ERROR("Unknown cmd: \"%s\".\n", cmd.c_str());
    }
  }

#define CHECK_TYPE(expected)                                           \
  if (!cmdproc.IsTypeValid(&expected)) {                               \
    ERROR("In %s: Type mismatch. expected %s got %s.\n", __FUNCTION__, \
          expected.ToString().c_str(),                                 \
          cmdproc.func_type()->ToString().c_str());                    \
    return;                                                            \
  }

#define ARG(ix, type, newvar)                                                 \
  type newvar;                                                                \
  if (!cmdproc.Get(ix, &newvar)) {                                            \
    ERROR("In %s: Argument %d cannot be converted to type %s.", __FUNCTION__, \
          ix, #type);                                                         \
    return;                                                                   \
  }

#define BEGIN \
  if (false) {}

#define TYPE(name) \
  else if (cmdproc.func_type() == &name)

#define END                                                       \
  else {                                                          \
    printf("In %s: Unexpected function type: %s\n", __FUNCTION__, \
        cmdproc.func_type()->ToString().c_str());                 \
  }

  void Handle_add(const CommandProcessor& cmdproc, Handle ret_handle) {
    CHECK_TYPE(TYPE_add_void_p_int32);
    ARG(0, void*, ptr);
    ARG(1, int32_t, addend);

    void* result = ((uint8_t*)ptr) + addend;
    RegisterHandle(ret_handle, result);
    printf("add(%p, %d) => %p (%d)\n", ptr, addend, result, ret_handle);
  }

  void Handle_arrayBufferCreate(const CommandProcessor& cmdproc,
                                Handle ret_handle) {
    CHECK_TYPE(TYPE_arrayBufferCreate);
    ARG(0, uint32_t, length);
    pp::VarArrayBuffer array_buffer(length);
    // Grab the id here because the var is detached in RegisterHandle.
    uint64_t id = array_buffer.pp_var().value.as_id;
    RegisterHandle(ret_handle, &array_buffer);
    printf("arrayBufferCreate(%d) => %lld (%d)\n", length, id, ret_handle);
  }

  void Handle_arrayBufferMap(const CommandProcessor& cmdproc,
                             Handle ret_handle) {
    CHECK_TYPE(TYPE_arrayBufferMap);
    ARG(0, pp::VarArrayBuffer, buf);
    void* ptr = buf.Map();
    RegisterHandle(ret_handle, ptr);
    printf("arrayBufferMap(%lld) => %p (%d)\n", buf.pp_var().value.as_id, ptr,
           ret_handle);
  }

  void Handle_deflate(const CommandProcessor& cmdproc, Handle ret_handle) {
    CHECK_TYPE(TYPE_deflate);
    ARG(0, z_stream*, stream);
    ARG(1, int, flush);
    int result = deflate(stream, flush);
    RegisterHandle(ret_handle, result);
    printf("deflate(%p, %d) => %d\n", stream, flush, result);
  }

  void Handle_deflateInit(const CommandProcessor& cmdproc, Handle ret_handle) {
    CHECK_TYPE(TYPE_deflate);
    ARG(0, z_stream*, stream);
    ARG(1, int, level);
    int result = deflateInit(stream, level);

    RegisterHandle(ret_handle, result);
    printf("deflateInit(%p, %d) => %d\n", stream, level, result);
  }

  void Handle_get(const CommandProcessor& cmdproc, Handle ret_handle) {
    BEGIN
      TYPE(TYPE_get_uint8_p) {
        ARG(0, uint8_t**, ptr);
        uint8_t* result = *ptr;
        RegisterHandle(ret_handle, result);
        printf("*(uint8_t**)%p => %p\n", ptr, result);
      }
      TYPE(TYPE_get_uint32) {
        ARG(0, uint32_t*, ptr);
        uint32_t result = *ptr;
        RegisterHandle(ret_handle, result);
        printf("*(uint32_t*)%p => %u\n", ptr, result);
      }
    END
  }

  void Handle_malloc(const CommandProcessor& cmdproc, Handle ret_handle) {
    CHECK_TYPE(TYPE_malloc);
    ARG(0, size_t, size);
    void* result = malloc(size);
    RegisterHandle(ret_handle, result);
    printf("malloc(%d) => %p (%d)\n", size, result, ret_handle);
  }

  void Handle_memcpy(const CommandProcessor& cmdproc, Handle ret_handle) {
    CHECK_TYPE(TYPE_memcpy);
    ARG(0, void*, dst);
    ARG(1, void*, src);
    ARG(2, size_t, size);
    memcpy(dst, src, size);
    printf("memcpy(%p, %p, %d)\n", dst, src, size);
  }

  void Handle_memset(const CommandProcessor& cmdproc, Handle ret_handle) {
    CHECK_TYPE(TYPE_memset);
    ARG(0, void*, buffer);
    ARG(1, int, value);
    ARG(2, size_t, size);
    memset(buffer, value, size);
    printf("memset(%p, %d, %d)\n", buffer, value, size);
  }

  void Handle_set(const CommandProcessor& cmdproc, Handle ret_handle) {
    BEGIN
      TYPE(TYPE_set_uint8_p) {
        ARG(0, uint8_t**, ptr);
        ARG(1, uint8_t*, value);
        *ptr = value;
        printf("*(uint8_t*)%p = %p\n", ptr, value);
      }
      TYPE(TYPE_set_uint32) {
        ARG(0, uint32_t*, ptr);
        ARG(1, uint32_t, value);
        *ptr = value;
        printf("*(uint32_t)%p = %d\n", ptr, value);
      }
    END
  }

  void Handle_sub(const CommandProcessor& cmdproc, Handle ret_handle) {
    BEGIN
      TYPE(TYPE_sub_int32) {
        ARG(0, int32_t, minuend);
        ARG(1, int32_t, subtrahend);
        int32_t result = minuend - subtrahend;
        RegisterHandle(ret_handle, result);
        printf("sub(%d, %d) => %d (%d)\n", minuend, subtrahend, result,
               ret_handle);
      }
      TYPE(TYPE_sub_uint32) {
        ARG(0, uint32_t, minuend);
        ARG(1, uint32_t, subtrahend);
        int32_t result = minuend - subtrahend;
        RegisterHandle(ret_handle, result);
        printf("sub(%u, %u) => %u (%d)\n", minuend, subtrahend, result,
               ret_handle);
      }
    END
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
