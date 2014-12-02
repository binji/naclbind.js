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
#include "handle.h"
#include "json.h"
#include "run.h"
#include "var.h"

class GeneratorCallbackTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    request_ = PP_MakeUndefined();
    response_ = PP_MakeUndefined();
  }

  virtual void TearDown() {
    nb_var_release(request_);
    nb_var_release(response_);
    EXPECT_EQ(NB_TRUE, fake_interface_check_no_references());
  }

  void RunTest(const char* request_json, const char* expected_response_json) {
    request_ = json_to_var(request_json);
    ASSERT_EQ(PP_VARTYPE_DICTIONARY, request_.type);

    ASSERT_EQ(NB_TRUE, nb_request_run(request_, &response_));

    char* response_json = var_to_json_flat(response_);
    EXPECT_STREQ(expected_response_json, response_json);
    free(response_json);
  }

 protected:
  struct PP_Var request_;
  struct PP_Var response_;
};

TEST_F(GeneratorCallbackTest, Basic) {
  const char *request_json =
    "{\"id\": 1,"
    " \"set\": {\"1\": [\"function\", 1]}}";
  const char* response_json = "{\"id\":1,\"values\":[]}\n";
  int32_t func_id;
  RunTest(request_json, response_json);
  EXPECT_EQ(NB_TRUE, nb_handle_get_func_id(1, &func_id));
  EXPECT_EQ(1, func_id);
  nb_handle_destroy(1);
}
