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
#include <ppapi/c/pp_var.h>
#include "bool.h"
#include "fake_interfaces.h"
#include "json.h"
#include "run.h"
#include "var.h"

void GeneratorTest::SetUp() {
  request_ = PP_MakeUndefined();
  response_ = PP_MakeUndefined();
}

void GeneratorTest::TearDown() {
  CleanUp();
  EXPECT_EQ(NB_TRUE, fake_var_check_no_references());
}

void GeneratorTest::RunTest(const char* request_json,
                            const char* expected_response_json) {
  CleanUp();
  SetUp();

  request_ = json_to_var(request_json);
  ASSERT_EQ(PP_VARTYPE_DICTIONARY, request_.type);

  ASSERT_EQ(NB_TRUE, nb_request_run(request_, &response_));

  char* response_json = var_to_json_flat(response_);
  EXPECT_STREQ(expected_response_json, response_json);
  free(response_json);
}

void GeneratorTest::CleanUp() {
  nb_var_release(request_);
  nb_var_release(response_);
}
