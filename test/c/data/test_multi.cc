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
#include "fake_interfaces.h"
#include "json.h"
#include "run.h"
#include "var.h"

class MultiTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    request_ = PP_MakeUndefined();
    response_ = PP_MakeUndefined();
    response_json_ = NULL;
  }

  virtual void TearDown() {
    free(response_json_);
    nb_var_release(request_);
    nb_var_release(response_);
    EXPECT_EQ(TRUE, fake_var_check_no_references());
  }

  void RunTest(const char* request_json, const char* expected_response_json) {
    request_ = json_to_var(request_json);
    ASSERT_EQ(PP_VARTYPE_DICTIONARY, request_.type);

    ASSERT_EQ(TRUE, nb_request_run(request_, &response_));

    response_json_ = var_to_json_flat(response_);
    EXPECT_STREQ(expected_response_json, response_json_);
  }

 private:
  struct PP_Var request_;
  struct PP_Var response_;
  char* response_json_;
};

TEST_F(MultiTest, MultipleCommands) {
  const char *request_json =
    "{\"id\": 1,"
    " \"set\": {\"1\": \"Hello\","
    "           \"2\": 6,"
    "           \"3\": 5},"
    " \"commands\": [{\"id\": 1, \"args\": [2], \"ret\": 4},"  // malloc
    "                {\"id\": 2, \"args\": [4, 1, 2]},"        // memcpy
    "                {\"id\": 4, \"args\": [4, 3]},"           // rot13
    "                {\"id\": 5, \"args\": [4], \"ret\": 5},"  // char_to_var
    "                {\"id\": 6, \"args\": [5]},"              // var_release
    "                {\"id\": 3, \"args\": [4]}],"             // free
    " \"get\": [5],"
    " \"destroy\": [1, 2, 3, 4, 5, 6]}";
  const char* response_json = "{\"id\":1,\"values\":[\"Uryyb\"]}\n";
  RunTest(request_json, response_json);
}
