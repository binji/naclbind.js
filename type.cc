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

#include "type.h"

#include <assert.h>
#include <stdio.h>

#include <zlib.h>

const int kNumTypes = 36;
Type* g_type_map[kNumTypes];

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


Type::Type(TypeId id) : id_(id) {
  assert(id >= 0 && id <= kNumTypes);
  assert(g_type_map[id] == NULL);
  g_type_map[id] = this;
}

// static
Type* Type::Get(TypeId id) {
  assert(id >= 0 && id <= kNumTypes);
  assert(g_type_map[id] != NULL);
  return g_type_map[id];
}

VoidType::VoidType(TypeId id) : Type(id) {}

PointerType::PointerType(TypeId id, Type* base_type)
    : Type(id), base_type_(base_type) {}

std::string PointerType::ToString() {
  return base_type_->ToString() + "*";
}

bool PointerType::GetValue(void* ptr, void** out_value) {
  *out_value = static_cast<void*>(ptr);
  return true;
}

StructType::StructType(TypeId id, const char* name, size_t size, int num_fields,
                       StructField* fields)
    : Type(id), name_(name), size_(size) {
  for (int i = 0; i < num_fields; ++i) {
    fields_.push_back(fields[i]);
  }
}

FunctionType::FunctionType(TypeId id, Type* ret_type)
    : Type(id), ret_type_(ret_type) {}

FunctionType::FunctionType(TypeId id, Type* ret_type, Type* arg0)
    : Type(id), ret_type_(ret_type) {
  arg_types_.push_back(arg0);
}

FunctionType::FunctionType(TypeId id, Type* ret_type, Type* arg0, Type* arg1)
    : Type(id), ret_type_(ret_type) {
  arg_types_.push_back(arg0);
  arg_types_.push_back(arg1);
}

FunctionType::FunctionType(TypeId id, Type* ret_type, Type* arg0, Type* arg1,
                           Type* arg2)
    : Type(id), ret_type_(ret_type) {
  arg_types_.push_back(arg0);
  arg_types_.push_back(arg1);
  arg_types_.push_back(arg2);
}

FunctionType::FunctionType(TypeId id, Type* ret_type, Type* arg0, Type* arg1,
                           Type* arg2, Type* arg3)
    : Type(id), ret_type_(ret_type) {
  arg_types_.push_back(arg0);
  arg_types_.push_back(arg1);
  arg_types_.push_back(arg2);
  arg_types_.push_back(arg3);
}

FunctionType::FunctionType(TypeId id, Type* ret_type, Type** arg_types)
    : Type(id), ret_type_(ret_type) {
  while (arg_types && *arg_types) {
    arg_types_.push_back(*arg_types);
    arg_types++;
  }
}

std::string FunctionType::ToString() {
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

Type* FunctionType::GetArgType(size_t ix) {
  if (ix >= arg_types_.size()) {
    return NULL;
  }
  return arg_types_[ix];
}

PepperType::PepperType(TypeId id, const char* name, PP_VarType var_type)
    : Type(id), name_(name), var_type_(var_type) {}

bool PepperType::GetValue(void* ptr, pp::VarArrayBuffer* out_value) {
  if (var_type_ != PP_VARTYPE_ARRAY_BUFFER) {
    return false;
  }
  pp::Var var(*static_cast<PP_Var*>(ptr));
  *out_value = pp::VarArrayBuffer(var);
  return true;
}

void PepperType::DestroyValue(void* ptr) {
  if (var_type_ != PP_VARTYPE_ARRAY_BUFFER) {
    return;
  }

  PP_Var* var_ptr = static_cast<PP_Var*>(ptr);
  // Pass the reference count to this var, it will be cleaned up at the
  // function scope exit.
  pp::Var var(pp::PASS_REF, *static_cast<PP_Var*>(ptr));
  delete var_ptr;
}

