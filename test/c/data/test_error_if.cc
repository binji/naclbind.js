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

#include "test_gen.h"
#include "json.h"
#include "run.h"
#include "var.h"

TEST_F(GeneratorTest, ErrorIf) {
  struct NB_Queue* message_queue = NULL;
  const char* request_json =
    "{\"id\": 1,"
    " \"commands\": ["
    "     {\"id\": 0, \"args\": [], \"ret\": 1},"   // get_int
    "     {\"id\": -1, \"args\": [1]},"             // error_if
    "     {\"id\": 1, \"args\": [], \"ret\": 2}],"  // do_stuff
    " \"get\": [1, 2]}";

  request_ = json_to_var(request_json);
  ASSERT_EQ(PP_VARTYPE_DICTIONARY, request_.type);
  ASSERT_EQ(NB_FALSE, nb_request_run(message_queue, request_, &response_));

  struct PP_Var keys = nb_var_dict_get_keys(response_);
  EXPECT_EQ(3U, nb_var_array_length(keys));
  nb_var_release(keys);

  // Check response is {"id": 1, "values": [1, undefined], "error": 1}
  // We can't use jsoncpp because it doesn't support undefined values.
  struct PP_Var id_var = nb_var_dict_get(response_, "id");
  EXPECT_EQ(PP_VARTYPE_INT32, id_var.type);
  EXPECT_EQ(1, id_var.value.as_int);

  struct PP_Var error_var = nb_var_dict_get(response_, "error");
  EXPECT_EQ(PP_VARTYPE_INT32, error_var.type);
  EXPECT_EQ(1, error_var.value.as_int);

  struct PP_Var values_var = nb_var_dict_get(response_, "values");
  EXPECT_EQ(PP_VARTYPE_ARRAY, values_var.type);
  EXPECT_EQ(2U, nb_var_array_length(values_var));

  struct PP_Var values_0_var = nb_var_array_get(values_var, 0);
  EXPECT_EQ(PP_VARTYPE_INT32, values_0_var.type);
  EXPECT_EQ(1, values_0_var.value.as_int);

  struct PP_Var values_1_var = nb_var_array_get(values_var, 1);
  EXPECT_EQ(PP_VARTYPE_UNDEFINED, values_1_var.type);

  nb_var_release(values_var);
}
