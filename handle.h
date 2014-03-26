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

#ifndef HANDLE_H_
#define HANDLE_H_

#include "type.h"

#include <map>

typedef int32_t Handle;

struct HandleObject {
  HandleObject() : ptr(NULL), type(NULL) {}
  HandleObject(void* ptr, Type* type) : ptr(ptr), type(type) {}

  void* ptr;
  Type* type;
};

typedef std::map<Handle, HandleObject> HandleMap;
extern HandleMap g_handle_map;

bool GetHandle(Handle handle, HandleObject* out_hobj);
template <typename T>
bool GetHandleValue(Handle handle, T* out_value);
void RegisterHandle(Handle handle, void* pointer, Type* type);
template <typename T>
void RegisterHandle(Handle handle, T arg);
template <typename T>
void RegisterHandle(Handle handle, T* pointer);
template <>
void RegisterHandle(Handle handle, pp::VarArrayBuffer* array_buffer);
void DestroyHandle(Handle handle);

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

#endif  // HANDLE_H_
