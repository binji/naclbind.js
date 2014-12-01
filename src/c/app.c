/* Copyright 2014 Ben Smith. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NB_ONE_FILE
#include "queue.h"
#include "response.h"
#include "run.h"
#include "var.h"
#endif

#include <ppapi/c/pp_errors.h>
#include <ppapi/c/pp_module.h>
#include <ppapi/c/ppb.h>
#include <ppapi/c/ppb_instance.h>
#include <ppapi/c/ppp.h>
#include <ppapi/c/ppp_instance.h>
#include <ppapi/c/ppp_messaging.h>
#include <pthread.h>

enum { NB_QUEUE_MAX_SIZE = 256 };

static PPB_GetInterface s_nb_get_browser_interface = NULL;
static pthread_t s_nb_thread_id;
static struct NB_Queue* s_nb_message_queue;

static PP_Bool nb_instance_did_create(PP_Instance,
                                      uint32_t,
                                      const char* [],
                                      const char* []);
static void nb_instance_did_destroy(PP_Instance);
static void nb_instance_did_change_view(PP_Instance, PP_Resource);
static void nb_instance_did_change_focus(PP_Instance, PP_Bool);
static PP_Bool nb_instance_handle_document_load(PP_Instance, PP_Resource);
static void nb_instance_handle_message(PP_Instance, struct PP_Var);
static void* nb_handle_message_thread(void*);

PP_EXPORT int32_t
    PPP_InitializeModule(PP_Module a_module_id, PPB_GetInterface get_browser) {
  s_nb_get_browser_interface = get_browser;
  return PP_OK;
}

PP_EXPORT const void* PPP_GetInterface(const char* interface_name) {
  if (strcmp(interface_name, PPP_INSTANCE_INTERFACE_1_1) == 0) {
    static PPP_Instance instance_interface = {
        &nb_instance_did_create,
        &nb_instance_did_destroy,
        &nb_instance_did_change_view,
        &nb_instance_did_change_focus,
        &nb_instance_handle_document_load,
    };
    return &instance_interface;
  } else if (strcmp(interface_name, PPP_MESSAGING_INTERFACE_1_0) == 0) {
    static PPP_Messaging messaging_interface = {
        &nb_instance_handle_message,
    };
    return &messaging_interface;
  }
  return NULL;
}

PP_EXPORT void PPP_ShutdownModule() {
}

PP_Bool nb_instance_did_create(PP_Instance instance,
                               uint32_t argc,
                               const char* argn[],
                               const char* argv[]) {
  nb_interfaces_init(instance, s_nb_get_browser_interface);
  s_nb_message_queue = nb_queue_create(NB_QUEUE_MAX_SIZE);
  pthread_create(&s_nb_thread_id, NULL, &nb_handle_message_thread, NULL);
  return PP_TRUE;
}

void nb_instance_did_destroy(PP_Instance instance) {
}
void nb_instance_did_change_view(PP_Instance instance, PP_Resource resource) {
}
void nb_instance_did_change_focus(PP_Instance instance, PP_Bool has_focus) {
}

PP_Bool nb_instance_handle_document_load(PP_Instance instance,
                                         PP_Resource resource) {
  return PP_FALSE;
}

void nb_instance_handle_message(PP_Instance instance, struct PP_Var var) {
  if (!nb_queue_enqueue(s_nb_message_queue, var)) {
    NB_ERROR("Warning: dropped message because the queue was full.");
  }
}

static void* nb_handle_message_thread(void* user_data) {
  while (1) {
    struct PP_Var request = nb_queue_dequeue(s_nb_message_queue);
    struct PP_Var response = PP_MakeUndefined();

    nb_request_run(request, &response);
    g_nb_ppb_messaging->PostMessage(g_nb_pp_instance, response);
    nb_var_release(response);
    nb_var_release(request);
  }
  return NULL;
}
