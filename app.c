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

#include <ppapi/c/pp_var.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/var.h>
#include <ppapi/utility/completion_callback_factory.h>

#ifdef WIN32
#undef PostMessage
// Allow 'this' in initializer list
#pragma warning(disable : 4355)
#endif

#include "commands.h"
#include "error.h"
#include "interfaces.h"
#include "message.h"
#include "var.h"

class Instance : public pp::Instance {
 public:
  explicit Instance(PP_Instance instance)
      : pp::Instance(instance), callback_factory_(this) {}

  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
    InitInterfaces(pp_instance(), pp::Module::Get()->get_browser_interface());
    return true;
  }

  virtual void HandleMessage(const pp::Var& var) {
    PP_Var pp_var = var.pp_var();
    Message* message = CreateMessage(pp_var);
    int32_t command_count = GetMessageCommandCount(message);
    for (int32_t i = 0; i < command_count; ++i) {
      Command* command = GetMessageCommand(message, i);
      HandleCommand(command);
      DestroyCommand(command);
    }

    int32_t ret_handle_count = GetMessageRetHandleCount(message);
    PP_Var values_var;
    CreateArrayVar(&values_var);
    for (int32_t i = 0; i < ret_handle_count; ++i) {
      Handle handle;
      Type type;
      if (!GetMessageRetHandle(message, i, &handle, &type)) {
        VERROR("Bad ret handle at index %d", i);
        continue;
      }

      PP_Var value_var;
      if (!HandleToVar(handle, &value_var)) {
        VERROR("Unable to convert handle %d (index %d) to var", handle, i);
        continue;
      }

      SetArrayVar(&values_var, i, value_var);
    }

    PP_Var response;
    CreateDictVar(&response);
    SetDictVar(&response, "id", PP_MakeInt32(message->id));
    SetDictVar(&response, "values", values_var);
    PostMessage(pp::Var(response));
    ReleaseVar(&response);

    DestroyMessage(message);
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

