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

HandleMap g_handle_map;

bool GetHandle(Handle handle, HandleObject* out_hobj) {
  HandleMap::iterator iter = g_handle_map.find(handle);
  if (iter == g_handle_map.end()) {
    return false;
  }

  *out_hobj = iter->second;
  return true;
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

void DestroyHandle(Handle handle) {
  HandleMap::iterator iter = g_handle_map.find(handle);
  if (iter == g_handle_map.end()) {
    ERROR("DestroyHandle: handle %d doesn't exist.\n", handle);
    return;
  }

  HandleObject& hobj = iter->second;
  hobj.type->DestroyValue(hobj.ptr);
  g_handle_map.erase(iter);
}

template <>
void RegisterHandle(Handle handle, pp::VarArrayBuffer* array_buffer) {
  PP_Var var = array_buffer->Detach();
  RegisterHandle(handle, new PP_Var(var), &TYPE_arrayBuffer);
}
