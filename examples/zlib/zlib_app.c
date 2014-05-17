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

#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <sys/mount.h>

#include <ppapi/c/pp_errors.h>
#include <ppapi/c/pp_module.h>
#include <ppapi/c/ppb.h>
#include <ppapi/c/ppb_instance.h>
#include <ppapi/c/ppb_var.h>
#include <ppapi/c/ppp.h>
#include <ppapi/c/ppp_instance.h>
#include <ppapi/c/ppp_messaging.h>

#include "nacl_io/nacl_io.h"

#include "bool.h"
#include "commands.h"
#include "error.h"
#include "interfaces.h"
#include "message.h"
#include "queue.h"
#include "var.h"

extern bool HandleZlibCommand(Command* command);

static PPB_GetInterface get_browser_interface = NULL;
static pthread_t g_handle_message_thread;


static void* HandleMessageThread(void*);
static void HandleMessage(struct PP_Var);


static PP_Bool Instance_DidCreate(PP_Instance instance,
                                  uint32_t argc,
                                  const char* argn[],
                                  const char* argv[]) {
  InitInterfaces(instance, get_browser_interface);
  nacl_io_init_ppapi(instance, get_browser_interface);
  umount("/");
  mount("", "/", "memfs", 0, "");
  InitializeMessageQueue();
  pthread_create(&g_handle_message_thread, NULL, &HandleMessageThread, NULL);

  return PP_TRUE;
}

static void* HandleMessageThread(void* user_data) {
  while (1) {
    struct PP_Var message = DequeueMessage();
    HandleMessage(message);
    ReleaseVar(&message);
  }
  return NULL;
}

static void HandleMessage(struct PP_Var var) {
  Message* message = CreateMessage(var);
  if (!message) {
    ERROR("Unable to create message.");
    return;
  }

  int32_t command_count = GetMessageCommandCount(message);
  for (int32_t i = 0; i < command_count; ++i) {
    Command* command = GetMessageCommand(message, i);
    if (!command) {
      VERROR("Unable to get command at index %d", i);
      continue;
    }

    if (!HandleZlibCommand(command)) {
      if (!HandleBuiltinCommand(command)) {
        VERROR("Unknown command: %s", command->command);
      }
    }

    DestroyCommand(command);
  }

  int32_t ret_handle_count = GetMessageRetHandleCount(message);
  struct PP_Var values_var;
  CreateArrayVar(&values_var);
  for (int32_t i = 0; i < ret_handle_count; ++i) {
    Handle handle;
    if (!GetMessageRetHandle(message, i, &handle)) {
      VERROR("Bad ret handle at index %d", i);
      continue;
    }

    struct PP_Var value_var;
    if (!HandleToVar(handle, &value_var)) {
      VERROR("Unable to convert handle %d (index %d) to var", handle, i);
      continue;
    }

    SetArrayVar(&values_var, i, value_var);
  }

  struct PP_Var response;
  CreateDictVar(&response);
  SetDictVar(&response, "id", PP_MakeInt32(message->id));
  SetDictVar(&response, "values", values_var);
  g_ppb_messaging->PostMessage(g_pp_instance, response);
  ReleaseVar(&response);

  DestroyMessage(message);
}



static void Instance_DidDestroy(PP_Instance instance) {}

static void Instance_DidChangeView(PP_Instance instance,
                                   PP_Resource view_resource) {}

static void Instance_DidChangeFocus(PP_Instance instance, PP_Bool has_focus) {}

static PP_Bool Instance_HandleDocumentLoad(PP_Instance instance,
                                           PP_Resource url_loader) {
  return PP_FALSE;
}

static void Messaging_HandleMessage(PP_Instance instance,
                                    struct PP_Var message) {
  AddRefVar(&message);
  if (!EnqueueMessage(message)) {
    ERROR("Warning: dropped message because the queue was full.");
  }
}


PP_EXPORT int32_t PPP_InitializeModule(PP_Module a_module_id,
                                       PPB_GetInterface get_browser) {
  get_browser_interface = get_browser;
  return PP_OK;
}

PP_EXPORT const void* PPP_GetInterface(const char* interface_name) {
  if (strcmp(interface_name, PPP_INSTANCE_INTERFACE_1_1) == 0) {
    static PPP_Instance instance_interface = {
      &Instance_DidCreate,
      &Instance_DidDestroy,
      &Instance_DidChangeView,
      &Instance_DidChangeFocus,
      &Instance_HandleDocumentLoad,
    };
    return &instance_interface;
  } else if (strcmp(interface_name, PPP_MESSAGING_INTERFACE_1_0) == 0) {
    static PPP_Messaging messaging_interface = {
      &Messaging_HandleMessage,
    };
    return &messaging_interface;
  }
  return NULL;
}

PP_EXPORT void PPP_ShutdownModule() {}
