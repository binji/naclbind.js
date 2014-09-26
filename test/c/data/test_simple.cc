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

#include <gtest/gtest.h>
#include <ppapi/c/pp_var.h>
#include "bool.h"
#include "json.h"
#include "run.h"
#include "var.h"

extern "C" {
int g_foo_called = 0;
}

TEST(SimpleTest, Basic) {
  const char* json = "{\"id\": 1, \"commands\": [{\"id\": 1, \"args\": []}]}";
  struct PP_Var request = json_to_var(json);
  ASSERT_EQ(PP_VARTYPE_DICTIONARY, request.type);

  struct PP_Var response;
  ASSERT_EQ(TRUE, nb_request_run(request, &response));

  char* response_json = var_to_json_flat(response);
  EXPECT_STREQ("{\"id\":1,\"values\":[]}\n", response_json);
  EXPECT_EQ(1, g_foo_called);

  nb_var_release(request);
  nb_var_release(response);
  free(response_json);
}
