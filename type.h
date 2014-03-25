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

#ifndef TYPE_H_
#define TYPE_H_

#include "error.h"

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include <ppapi/cpp/var_array_buffer.h>

typedef int32_t TypeId;
class Type;

class Type {
 public:
  explicit Type(TypeId id);
  virtual ~Type() {}
  virtual size_t Size() = 0;
  virtual std::string ToString() = 0;

  TypeId id() { return id_; }

  static Type* Get(TypeId id);

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

  virtual void DestroyValue(void* ptr) {}

 private:
  TypeId id_;
};

class VoidType : public Type {
 public:
  explicit VoidType(TypeId id);

  virtual size_t Size() { return 0; }
  virtual std::string ToString() { return "void"; }
};

template <typename T>
class PrimitiveType : public Type {
 public:
  PrimitiveType(TypeId id, const char* name)
      : Type(id), name_(name) {}

  virtual size_t Size() { return sizeof(T); }
  virtual std::string ToString() { return name_; }

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

  virtual void DestroyValue(void* ptr) {
    delete static_cast<T*>(ptr);
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
  PointerType(TypeId id, Type* base_type);
  virtual size_t Size() { return 4; }
  virtual std::string ToString();
  virtual bool GetValue(void* ptr, void** out_value);

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
             StructField* fields);
  virtual size_t Size() { return size_; }
  virtual std::string ToString() { return name_; }

 private:
  const char* name_;
  size_t size_;
  std::vector<StructField> fields_;
};

class FunctionType : public Type {
 public:
  FunctionType(TypeId id, Type* ret_type);
  FunctionType(TypeId id, Type* ret_type, Type* arg0);
  FunctionType(TypeId id, Type* ret_type, Type* arg0, Type* arg1);
  FunctionType(TypeId id, Type* ret_type, Type* arg0, Type* arg1, Type* arg2);
  FunctionType(TypeId id, Type* ret_type, Type* arg0, Type* arg1, Type* arg2,
               Type* arg3);
  FunctionType(TypeId id, Type* ret_type, Type** arg_types);
  virtual size_t Size() { return 4; }
  virtual std::string ToString();
  Type* ret_type() { return ret_type_; }
  Type* GetArgType(size_t ix);

 private:
  Type* ret_type_;
  std::vector<Type*> arg_types_;
};

class PepperType : public Type {
 public:
  PepperType(TypeId id, const char* name, PP_VarType var_type);
  virtual size_t Size() { return sizeof(pp::Var); }
  virtual std::string ToString() { return name_; }

  PP_VarType var_type() const { return var_type_; }

  bool GetValue(void* ptr, pp::VarArrayBuffer* out_value);
  void DestroyValue(void* ptr);

 private:
  const char* name_;
  PP_VarType var_type_;
};


// Builtin types.
extern VoidType TYPE_void;
extern PrimitiveType<int8_t> TYPE_int8;
extern PrimitiveType<uint8_t> TYPE_uint8;
extern PrimitiveType<int16_t> TYPE_int16;
extern PrimitiveType<uint16_t> TYPE_uint16;
extern PrimitiveType<int32_t> TYPE_int32;
extern PrimitiveType<uint32_t> TYPE_uint32;
extern PrimitiveType<int64_t> TYPE_int64;
extern PrimitiveType<int64_t> TYPE_uint64;
extern PrimitiveType<float> TYPE_float32;
extern PrimitiveType<double> TYPE_float64;
extern PrimitiveType<size_t> TYPE_size_t;
extern PointerType TYPE_void_p;
extern PointerType TYPE_uint8_p;
extern PointerType TYPE_uint8_pp;
extern PointerType TYPE_uint32_p;
extern PepperType TYPE_arrayBuffer;
extern PepperType TYPE_array;
extern PepperType TYPE_dictionary;
extern FunctionType TYPE_malloc;
extern FunctionType TYPE_memset;
extern FunctionType TYPE_memcpy;
extern FunctionType TYPE_add_void_p_int32;
extern FunctionType TYPE_set_uint8_p;
extern FunctionType TYPE_set_uint32;
extern FunctionType TYPE_get_uint8_p;
extern FunctionType TYPE_get_uint32;
extern FunctionType TYPE_sub_int32;
extern FunctionType TYPE_sub_uint32;
extern FunctionType TYPE_arrayBufferCreate;
extern FunctionType TYPE_arrayBufferMap;
extern FunctionType TYPE_arrayBufferUnmap;

#endif  // TYPE_H_
