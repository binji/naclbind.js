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
#include "request.h"
#include "var.h"

class RequestTest : public ::testing::Test {
 public:
  RequestTest() : request(NULL) {}

  void JsonToRequest(const char* json) {
    if (request) {
      nb_request_destroy(request);
    }

    struct PP_Var var = json_to_var(json);
    ASSERT_EQ(PP_VARTYPE_DICTIONARY, var.type)
        << "  Failed to parse json:\n  " << json;

    request = nb_request_create(var);
    nb_var_release(var);
  }

  virtual void TearDown() {
    if (request) {
      nb_request_destroy(request);
    }

    EXPECT_EQ(NB_TRUE, fake_interface_check_no_references());
  }

 protected:
  NB_Request* request;
};

NB_Request* NULL_REQUEST = NULL;

TEST_F(RequestTest, Valid) {
  const char* valid_requests[] = {
    "{\"id\": 1}",
    "{\"id\": 1, \"get\": []}",
    "{\"id\": 1, \"get\": [1]}",
    "{\"id\": 1, \"set\": {}}",
    "{\"id\": 1, \"set\": {\"1\": 4}}",
    "{\"id\": 1, \"set\": {\"1\": 3.5}}",
    "{\"id\": 1, \"set\": {\"1\": \"hi\"}}",
    "{\"id\": 1, \"set\": {\"1\": null}}",
    "{\"id\": 1, \"set\": {\"1\": [\"long\", 0, 256]}}",
    "{\"id\": 1, \"set\": {\"1\": [\"function\", 20]}}",
    "{\"id\": 1, \"commands\": [{\"id\": 1, \"args\": [2, 3]}]}",
    "{\"id\": 1, \"commands\": [{\"id\": 1, \"args\": [2, 3], \"ret\": 4}]}",
    "{\"id\": 1, \"get\": [10], \"destroy\": []}",
    "{\"id\": 1, \"get\": [10], \"destroy\": [1, 5, 10]}",
    "{\"id\": 1, \"get\": [], \"set\": {}, \"destroy\": [], \"commands\": []}",
    NULL
  };

  for (int i = 0; valid_requests[i]; ++i) {
    const char* json = valid_requests[i];
    JsonToRequest(json);
    EXPECT_NE(NULL_REQUEST, request) << "Expected valid: " << json;
  }
}

TEST_F(RequestTest, Invalid) {
  const char* invalid_requests[] = {
    // Missing "id"
    "{}",
    // "id" can't be < 0
    "{\"id\": 0}",
    // "id" must be string
    "{\"id\": \"foo\"}",
    // "get" must be array
    "{\"id\": 1, \"get\": {}}",
    // "get" must be array of ints
    "{\"id\": 1, \"get\": [4.3]}",
    // "set" must be dictionary
    "{\"id\": 1, \"set\": [1, 2]}",
    // "set" keys must be ints
    "{\"id\": 1, \"set\": {\"hi\": 3}}",
    // "set" values can't be object
    "{\"id\": 1, \"set\": {\"1\": {}}}",
    // "set" values array must start with string tag
    "{\"id\": 1, \"set\": {\"1\": [1]}}",
    // "set" values array string tag must be valid tag
    "{\"id\": 1, \"set\": {\"1\": [\"foo\", 1, 2]}}",
    // "set" values array with tag "long" must have len 3
    "{\"id\": 1, \"set\": {\"1\": [\"long\", 1]}}",
    // "set" values array with tag "function" must have len 2
    "{\"id\": 1, \"set\": {\"1\": [\"function\"]}}",
    // "destroy" must be array
    "{\"id\": 1, \"destroy\": {}}",
    // "destroy" must be array of ints
    "{\"id\": 1, \"destroy\": [null]}",
    // "commands" must be array
    "{\"id\": 1, \"commands\": null}",
    // "commands" must be array of dicts
    "{\"id\": 1, \"commands\": [14]}",
    // Missing \"id\" and \"args\"
    "{\"id\": 1, \"commands\": [{}]}",
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

  for (int i = 0; invalid_requests[i]; ++i) {
    const char* json = invalid_requests[i];
    JsonToRequest(json);
    EXPECT_EQ(NULL_REQUEST, request) << "Expected invalid: " << json;
  }
}

TEST_F(RequestTest, Id) {
  const char* json = "{\"id\": 1}";
  JsonToRequest(json);
  ASSERT_NE(NULL_REQUEST, request) << "Expected valid: " << json;

  EXPECT_EQ(1, nb_request_id(request));
}

TEST_F(RequestTest, SetHandles) {
  const char* json = "{\"id\": 1, \"set\": {\"1\": 4, \"2\": 5}}";
  JsonToRequest(json);
  ASSERT_NE(NULL_REQUEST, request) << "Expected valid: " << json;

  EXPECT_EQ(1, nb_request_id(request));
  EXPECT_EQ(2, nb_request_sethandles_count(request));

  NB_Handle handle;
  struct PP_Var value;

  nb_request_sethandle(request, 0, &handle, &value);
  EXPECT_EQ(1, handle);
  EXPECT_EQ(PP_VARTYPE_INT32, value.type);
  EXPECT_EQ(4, value.value.as_int);
  nb_var_release(value);

  nb_request_sethandle(request, 1, &handle, &value);
  EXPECT_EQ(2, handle);
  EXPECT_EQ(PP_VARTYPE_INT32, value.type);
  EXPECT_EQ(5, value.value.as_int);
  nb_var_release(value);
}

TEST_F(RequestTest, SetHandles_String) {
  const char* json = "{\"id\": 1, \"set\": {\"1\": \"Hi\"}}";
  JsonToRequest(json);
  ASSERT_NE(NULL_REQUEST, request) << "Expected valid: " << json;

  EXPECT_EQ(1, nb_request_id(request));
  EXPECT_EQ(1, nb_request_sethandles_count(request));

  NB_Handle handle;
  struct PP_Var value;

  nb_request_sethandle(request, 0, &handle, &value);
  EXPECT_EQ(1, handle);
  EXPECT_EQ(PP_VARTYPE_STRING, value.type);

  const char* s;
  uint32_t len;
  EXPECT_EQ(NB_TRUE, nb_var_string(value, &s, &len));
  EXPECT_EQ(2, len);
  EXPECT_STREQ("Hi", s);
  nb_var_release(value);
}

TEST_F(RequestTest, SetHandles_Null) {
  const char* json = "{\"id\": 1, \"set\": {\"1\": null}}";
  JsonToRequest(json);
  ASSERT_NE(NULL_REQUEST, request) << "Expected valid: " << json;

  EXPECT_EQ(1, nb_request_id(request));
  EXPECT_EQ(1, nb_request_sethandles_count(request));

  NB_Handle handle;
  struct PP_Var value;

  nb_request_sethandle(request, 0, &handle, &value);
  EXPECT_EQ(1, handle);
  EXPECT_EQ(PP_VARTYPE_NULL, value.type);
  nb_var_release(value);
}

TEST_F(RequestTest, SetHandles_Long) {
  const char* json = "{\"id\": 1, \"set\": {\"1\": [\"long\", 0, 1]}}";
  JsonToRequest(json);
  ASSERT_NE(NULL_REQUEST, request) << "Expected valid: " << json;

  EXPECT_EQ(1, nb_request_id(request));
  EXPECT_EQ(1, nb_request_sethandles_count(request));

  NB_Handle handle;
  struct PP_Var value;
  struct PP_Var tag;
  const char* tag_str;
  uint32_t tag_len;

  nb_request_sethandle(request, 0, &handle, &value);
  EXPECT_EQ(1, handle);
  EXPECT_EQ(PP_VARTYPE_ARRAY, value.type);
  EXPECT_EQ(3, nb_var_array_length(value));
  tag = nb_var_array_get(value, 0);
  EXPECT_EQ(NB_TRUE, nb_var_string(tag, &tag_str, &tag_len));
  EXPECT_EQ(4, tag_len);
  EXPECT_EQ(0, strcmp(tag_str, "long"));
  EXPECT_EQ(PP_VARTYPE_INT32, nb_var_array_get(value, 1).type);
  EXPECT_EQ(0, nb_var_array_get(value, 1).value.as_int);
  EXPECT_EQ(PP_VARTYPE_INT32, nb_var_array_get(value, 2).type);
  EXPECT_EQ(1, nb_var_array_get(value, 2).value.as_int);
  nb_var_release(tag);
  nb_var_release(value);
}

TEST_F(RequestTest, SetHandles_Function) {
  const char* json = "{\"id\": 1, \"set\": {\"1\": [\"function\", 1]}}";
  JsonToRequest(json);
  ASSERT_NE(NULL_REQUEST, request) << "Expected valid: " << json;

  EXPECT_EQ(1, nb_request_id(request));
  EXPECT_EQ(1, nb_request_sethandles_count(request));

  NB_Handle handle;
  struct PP_Var value;
  struct PP_Var tag;
  const char* tag_str;
  uint32_t tag_len;

  nb_request_sethandle(request, 0, &handle, &value);
  EXPECT_EQ(1, handle);
  EXPECT_EQ(PP_VARTYPE_ARRAY, value.type);
  EXPECT_EQ(2, nb_var_array_length(value));
  tag = nb_var_array_get(value, 0);
  EXPECT_EQ(NB_TRUE, nb_var_string(tag, &tag_str, &tag_len));
  EXPECT_EQ(8, tag_len);
  EXPECT_EQ(0, strcmp(tag_str, "function"));
  EXPECT_EQ(PP_VARTYPE_INT32, nb_var_array_get(value, 1).type);
  EXPECT_EQ(1, nb_var_array_get(value, 1).value.as_int);
  nb_var_release(tag);
  nb_var_release(value);
}

TEST_F(RequestTest, GetHandles) {
  const char* json = "{\"id\": 1, \"get\": [4, 5, 100]}";
  JsonToRequest(json);
  ASSERT_NE(NULL_REQUEST, request) << "Expected valid: " << json;

  EXPECT_EQ(1, nb_request_id(request));
  EXPECT_EQ(3, nb_request_gethandles_count(request));

  EXPECT_EQ(4, nb_request_gethandle(request, 0));
  EXPECT_EQ(5, nb_request_gethandle(request, 1));
  EXPECT_EQ(100, nb_request_gethandle(request, 2));
}

TEST_F(RequestTest, DestroyHandles) {
  const char* json = "{\"id\": 1, \"destroy\": [4, 5, 100]}";
  JsonToRequest(json);
  ASSERT_NE(NULL_REQUEST, request) << "Expected valid: " << json;

  EXPECT_EQ(1, nb_request_id(request));
  EXPECT_EQ(3, nb_request_destroyhandles_count(request));

  EXPECT_EQ(4, nb_request_destroyhandle(request, 0));
  EXPECT_EQ(5, nb_request_destroyhandle(request, 1));
  EXPECT_EQ(100, nb_request_destroyhandle(request, 2));
}

TEST_F(RequestTest, Commands) {
  const char* json =
      "{\"id\": 1, \"commands\": [{\"id\": 1, \"args\": [42, 3], \"ret\": 5}]}";
  JsonToRequest(json);
  ASSERT_NE(NULL_REQUEST, request) << "Expected valid: " << json;

  EXPECT_EQ(1, nb_request_id(request));
  EXPECT_EQ(1, nb_request_commands_count(request));

  EXPECT_EQ(1, nb_request_command_function(request, 0));
  EXPECT_EQ(2, nb_request_command_arg_count(request, 0));
  EXPECT_EQ(42, nb_request_command_arg(request, 0, 0));
  EXPECT_EQ(3, nb_request_command_arg(request, 0, 1));
  EXPECT_EQ(NB_TRUE, nb_request_command_has_ret(request, 0));
  EXPECT_EQ(5, nb_request_command_ret(request, 0));
}
