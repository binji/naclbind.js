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

#include <string>
#include <map>

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

#define VAR_INT(var, key) \
  int32_t key; \
  do { \
    pp::Var tempvar(var.Get(#key)); \
    if (!tempvar.is_int()) { \
      printf(#key " is not an int.\n"); \
      return; \
    } \
    key = tempvar.AsInt(); \
  } while(0)

#define VAR_STRING(var, key) \
  std::string key; \
  do { \
    pp::Var tempvar(var.Get(#key)); \
    if (!tempvar.is_string()) { \
      printf(#key " is not an string.\n"); \
      return; \
    } \
    key = tempvar.AsString(); \
  } while(0)

#define VAR_ARRAY(var, key) \
  pp::VarArray key; \
  do { \
    pp::Var tempvar(var.Get(#key)); \
    if (!tempvar.is_array()) { \
      printf(#key " is not an array.\n"); \
      return; \
    } \
    key = pp::VarArray(tempvar); \
  } while(0)

#define ARG_INT(var, ix, newvar) \
  int32_t newvar; \
  do { \
    pp::Var tempvar(var.Get(ix)); \
    if (!tempvar.is_int()) { \
      printf("Argument %d is not an int.\n", ix); \
      return; \
    } \
    newvar = tempvar.AsInt(); \
  } while(0)

#define ARG_HANDLE(var, ix, newvar) \
  void* newvar; \
  do { \
    ARG_INT(var, ix, newvar##int); \
    newvar = GetHandle(newvar##int); \
    if (!newvar) { \
      printf("Argument %d is not an valid handle.\n", ix); \
      return; \
    } \
  } while(0)


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
    VAR_INT(dictionary, id);
    VAR_ARRAY(dictionary, msgs);

    for (uint32_t i = 0; i < msgs.GetLength(); ++i) {
      HandleCommand(msgs.Get(i));
    }
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

  void HandleCommand(const pp::Var& msg) {
    VAR_DICTIONARY(msg, dictionary);
    VAR_STRING(dictionary, cmd);
    VAR_ARRAY(dictionary, args);
    VAR_INT(dictionary, ret);

    if (cmd == "malloc") {
      HandleMalloc(ret, args);
    } else if (cmd == "memset") {
      HandleMemset(ret, args);
    } else if (cmd == "memcpy") {
      HandleMemcpy(ret, args);
    }
  }

  void HandleMalloc(Handle ret_handle, const pp::VarArray& args) {
    ARG_INT(args, 0, size);
    void* result = malloc(size);
    RegisterHandle(ret_handle, result);
  }

  void HandleMemset(Handle ret_handle, const pp::VarArray& args) {
    ARG_HANDLE(args, 0, buffer);
    ARG_INT(args, 1, value);
    ARG_INT(args, 2, size);
    memset(buffer, value, size);
  }

  void HandleMemcpy(Handle ret_handle, const pp::VarArray& args) {
    ARG_HANDLE(args, 0, dst);
    ARG_HANDLE(args, 1, src);
    ARG_INT(args, 2, size);
    memcpy(dst, src, size);
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
