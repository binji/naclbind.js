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

#include "gtest/gtest.h"

#include <ppapi/c/pp_var.h>

#include "json.h"
#include "message.h"
#include "var.h"

TEST(Message, Valid) {
  const char* valid_messages[] = {
    "{\"id\": 1}",
    "{\"id\": 1, \"get\": []}",
    "{\"id\": 1, \"get\": [1]}",
    "{\"id\": 1, \"set\": {}}",
    "{\"id\": 1, \"set\": {\"1\": 4}}",
    "{\"id\": 1, \"commands\": [{\"id\": 1, \"args\": [2, 3]}]}",
    "{\"id\": 1, \"get\": [10], \"destroy\": []}",
    "{\"id\": 1, \"get\": [10], \"destroy\": [1, 5, 10]}",
    "{\"id\": 1, \"get\": [], \"set\": {}, \"destroy\": [], \"commands\": []}",
    NULL
  };

  for (int i = 0; valid_messages[i]; ++i) {
    const char* json = valid_messages[i];
    struct PP_Var var = json_to_var(json);
    struct Message* message = nb_message_create(var);
    EXPECT_NE((struct Message*) NULL, message) << "Expected valid: " << json;
    nb_var_release(var);
    nb_message_destroy(message);
  }
}

TEST(Message, Invalid) {
  const char* invalid_messages[] = {
    "{}",  // Missing "id"
    "{\"id\": 0}",  // "id" can't be < 0
    "{\"id\": \"foo\"}",  // "id" must be string
    "{\"id\": 1, \"get\": {}}",  // "get" must be array
    "{\"id\": 1, \"get\": [4.3]}",  // "get" must be array of ints
    "{\"id\": 1, \"set\": [1, 2]}",  // "set" must be dictionary
    "{\"id\": 1, \"set\": {\"hi\": 3}}",  // "set" keys must be ints
    // TODO(binji): relax this restriction.
    "{\"id\": 1, \"set\": {\"1\": \"yo\"}}",  // "set" values must be number
    "{\"id\": 1, \"destroy\": {}}",  // "destroy" must be array
    "{\"id\": 1, \"destroy\": [null]}",  // "destroy" must be array of ints
    "{\"id\": 1, \"commands\": null}",  // "commands" must be array
    "{\"id\": 1, \"commands\": [14]}",  // "commands" must be array of dicts
    "{\"id\": 1, \"commands\": [{}]}",  // Missing \"id\" and \"args\"
    // "id" must be int
    "{\"id\": 1, \"commands\": [{\"id\": \"bye\", \"args\":[]]}",
    // Missing "args"
    "{\"id\": 1, \"commands\": [{\"id\": 1}}",
    // "args" must be array
    "{\"id\": 1, \"commands\": [{\"id\": 1, \"args\":{}]}",
    // "args" must be array of int
    "{\"id\": 1, \"commands\": [{\"id\": 1, \"args\":[null]]}",
    // "ret" must be int
    "{\"id\": 1, \"commands\": [{\"id\": 1, \"args\":[], \"ret\": null]}",
    NULL
  };

  for (int i = 0; invalid_messages[i]; ++i) {
    const char* json = invalid_messages[i];
    struct PP_Var var = json_to_var(json);
    struct Message* message = nb_message_create(var);
    EXPECT_EQ((struct Message*) NULL, message) << "Expected invalid: " << json;
    nb_var_release(var);
  }
}
