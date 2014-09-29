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
#include "fake_interfaces.h"
#include "error.h"
#include "json.h"
#include "message.h"
#include "var.h"

class MessageTest : public ::testing::Test {
 public:
  MessageTest() : message(NULL) {}

  void JsonToMessage(const char* json) {
    if (message) {
      nb_message_destroy(message);
    }

    struct PP_Var var = json_to_var(json);
    ASSERT_EQ(PP_VARTYPE_DICTIONARY, var.type)
        << "  Failed to parse json:\n  " << json;

    message = nb_message_create(var);
    nb_var_release(var);
  }

  virtual void TearDown() {
    if (message) {
      nb_message_destroy(message);
    }

    EXPECT_EQ(TRUE, fake_var_check_no_references());
  }

 protected:
  Message* message;
};

Message* NULL_MESSAGE = NULL;

TEST_F(MessageTest, Valid) {
  const char* valid_messages[] = {
    "{\"id\": 1}",
    "{\"id\": 1, \"get\": []}",
    "{\"id\": 1, \"get\": [1]}",
    "{\"id\": 1, \"set\": {}}",
    "{\"id\": 1, \"set\": {\"1\": 4}}",
    "{\"id\": 1, \"set\": {\"1\": 3.5}}",
    "{\"id\": 1, \"set\": {\"1\": \"hi\"}}",
    "{\"id\": 1, \"set\": {\"1\": null}}",
    "{\"id\": 1, \"commands\": [{\"id\": 1, \"args\": [2, 3]}]}",
    "{\"id\": 1, \"commands\": [{\"id\": 1, \"args\": [2, 3], \"ret\": 4}]}",
    "{\"id\": 1, \"get\": [10], \"destroy\": []}",
    "{\"id\": 1, \"get\": [10], \"destroy\": [1, 5, 10]}",
    "{\"id\": 1, \"get\": [], \"set\": {}, \"destroy\": [], \"commands\": []}",
    NULL
  };

  for (int i = 0; valid_messages[i]; ++i) {
    const char* json = valid_messages[i];
    JsonToMessage(json);
    EXPECT_NE(NULL_MESSAGE, message) << "Expected valid: " << json;
  }
}

TEST_F(MessageTest, Invalid) {
  const char* invalid_messages[] = {
    "{}",  // Missing "id"
    "{\"id\": 0}",  // "id" can't be < 0
    "{\"id\": \"foo\"}",  // "id" must be string
    "{\"id\": 1, \"get\": {}}",  // "get" must be array
    "{\"id\": 1, \"get\": [4.3]}",  // "get" must be array of ints
    "{\"id\": 1, \"set\": [1, 2]}",  // "set" must be dictionary
    "{\"id\": 1, \"set\": {\"hi\": 3}}",  // "set" keys must be ints
    // "set" values must be number/string
    // TODO(binji): relax this restriction.
    "{\"id\": 1, \"set\": {\"1\": [1, 2]}}",
    "{\"id\": 1, \"destroy\": {}}",  // "destroy" must be array
    "{\"id\": 1, \"destroy\": [null]}",  // "destroy" must be array of ints
    "{\"id\": 1, \"commands\": null}",  // "commands" must be array
    "{\"id\": 1, \"commands\": [14]}",  // "commands" must be array of dicts
    "{\"id\": 1, \"commands\": [{}]}",  // Missing \"id\" and \"args\"
    // "id" must be int
    "{\"id\": 1, \"commands\": [{\"id\": \"bye\", \"args\":[]}]}",
    // Missing "args"
    "{\"id\": 1, \"commands\": [{\"id\": 1}]}",
    // "args" must be array
    "{\"id\": 1, \"commands\": [{\"id\": 1, \"args\":{}}]}",
    // "args" must be array of int
    "{\"id\": 1, \"commands\": [{\"id\": 1, \"args\":[null]}]}",
    // "ret" must be int
    "{\"id\": 1, \"commands\": [{\"id\": 1, \"args\":[], \"ret\": null}]}",
    NULL
  };

  for (int i = 0; invalid_messages[i]; ++i) {
    const char* json = invalid_messages[i];
    JsonToMessage(json);
    EXPECT_EQ(NULL_MESSAGE, message) << "Expected invalid: " << json;
  }
}

TEST_F(MessageTest, Id) {
  const char* json = "{\"id\": 1}";
  JsonToMessage(json);
  ASSERT_NE(NULL_MESSAGE, message) << "Expected valid: " << json;

  EXPECT_EQ(1, nb_message_id(message));
}

TEST_F(MessageTest, SetHandles) {
  const char* json = "{\"id\": 1, \"set\": {\"1\": 4, \"2\": 5}}";
  JsonToMessage(json);
  ASSERT_NE(NULL_MESSAGE, message) << "Expected valid: " << json;

  EXPECT_EQ(1, nb_message_id(message));
  EXPECT_EQ(2, nb_message_sethandles_count(message));

  Handle handle;
  struct PP_Var value;

  nb_message_sethandle(message, 0, &handle, &value);
  EXPECT_EQ(1, handle);
  EXPECT_EQ(PP_VARTYPE_INT32, value.type);
  EXPECT_EQ(4, value.value.as_int);
  nb_var_release(value);

  nb_message_sethandle(message, 1, &handle, &value);
  EXPECT_EQ(2, handle);
  EXPECT_EQ(PP_VARTYPE_INT32, value.type);
  EXPECT_EQ(5, value.value.as_int);
  nb_var_release(value);
}

TEST_F(MessageTest, GetHandles) {
  const char* json = "{\"id\": 1, \"get\": [4, 5, 100]}";
  JsonToMessage(json);
  ASSERT_NE(NULL_MESSAGE, message) << "Expected valid: " << json;

  EXPECT_EQ(1, nb_message_id(message));
  EXPECT_EQ(3, nb_message_gethandles_count(message));

  EXPECT_EQ(4, nb_message_gethandle(message, 0));
  EXPECT_EQ(5, nb_message_gethandle(message, 1));
  EXPECT_EQ(100, nb_message_gethandle(message, 2));
}

TEST_F(MessageTest, DestroyHandles) {
  const char* json = "{\"id\": 1, \"destroy\": [4, 5, 100]}";
  JsonToMessage(json);
  ASSERT_NE(NULL_MESSAGE, message) << "Expected valid: " << json;

  EXPECT_EQ(1, nb_message_id(message));
  EXPECT_EQ(3, nb_message_destroyhandles_count(message));

  EXPECT_EQ(4, nb_message_destroyhandle(message, 0));
  EXPECT_EQ(5, nb_message_destroyhandle(message, 1));
  EXPECT_EQ(100, nb_message_destroyhandle(message, 2));
}

TEST_F(MessageTest, Commands) {
  const char* json =
      "{\"id\": 1, \"commands\": [{\"id\": 1, \"args\": [42, 3], \"ret\": 5}]}";
  JsonToMessage(json);
  ASSERT_NE(NULL_MESSAGE, message) << "Expected valid: " << json;

  EXPECT_EQ(1, nb_message_id(message));
  EXPECT_EQ(1, nb_message_commands_count(message));

  EXPECT_EQ(1, nb_message_command_function(message, 0));
  EXPECT_EQ(2, nb_message_command_arg_count(message, 0));
  EXPECT_EQ(42, nb_message_command_arg(message, 0, 0));
  EXPECT_EQ(3, nb_message_command_arg(message, 0, 1));
  EXPECT_EQ(TRUE, nb_message_command_has_ret(message, 0));
  EXPECT_EQ(5, nb_message_command_ret(message, 0));
}
