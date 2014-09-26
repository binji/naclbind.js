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

#define REQUEST_JSON(fn, value) \
    "{\"id\": 1," \
    " \"set\": {\"1\": " #value "}," \
    " \"commands\": [{\"id\": " #fn ", \"args\": [1], \"ret\": 2}]," \
    " \"get\": [2]," \
    " \"destroy\": [1, 2]" \
    "}"

#define RESPONSE_JSON(value) \
    "{\"id\":1,\"values\":[" #value "]}\n"

class PrimitivesTest : public ::testing::Test {
 public:
  PrimitivesTest() {}

  void RunTest(const char* request_json, const char* expected_response_json) {
    struct PP_Var request = json_to_var(request_json);
    ASSERT_EQ(PP_VARTYPE_DICTIONARY, request.type);

    struct PP_Var response;
    ASSERT_EQ(TRUE, nb_request_run(request, &response));

    char* response_json = var_to_json_flat(response);
    EXPECT_STREQ(expected_response_json, response_json);

    nb_var_release(request);
    nb_var_release(response);
    free(response_json);
  }

  virtual void TearDown() {
    EXPECT_EQ(TRUE, fake_var_check_no_references());
  }
};

TEST_F(PrimitivesTest, Char) {
  RunTest(REQUEST_JSON(1, 42), RESPONSE_JSON(42));
}

TEST_F(PrimitivesTest, Schar) {
  RunTest(REQUEST_JSON(2, -42), RESPONSE_JSON(-42));
}

TEST_F(PrimitivesTest, Uchar) {
  RunTest(REQUEST_JSON(3, 142), RESPONSE_JSON(142));
}

TEST_F(PrimitivesTest, Short) {
  RunTest(REQUEST_JSON(4, -4200), RESPONSE_JSON(-4200));
}

TEST_F(PrimitivesTest, Ushort) {
  RunTest(REQUEST_JSON(5, 14200), RESPONSE_JSON(14200));
}

TEST_F(PrimitivesTest, Int) {
  RunTest(REQUEST_JSON(6, -420000), RESPONSE_JSON(-420000));
}

TEST_F(PrimitivesTest, Uint) {
  RunTest(REQUEST_JSON(7, 1420000), RESPONSE_JSON(1420000));
}

TEST_F(PrimitivesTest, Long) {
  RunTest(REQUEST_JSON(8, -420000), RESPONSE_JSON(-420000));
}

TEST_F(PrimitivesTest, Ulong) {
  RunTest(REQUEST_JSON(9, 1420000), RESPONSE_JSON(1420000));
}

// TODO(binji): int64 values are not yet implemented
TEST_F(PrimitivesTest, DISABLED_LongLong) {
  RunTest(REQUEST_JSON(10, -420000), RESPONSE_JSON(-420000));
}

// TODO(binji): int64 values are not yet implemented
TEST_F(PrimitivesTest, DISABLED_UlongLong) {
  RunTest(REQUEST_JSON(11, 1420000), RESPONSE_JSON(1420000));
}

TEST_F(PrimitivesTest, Float) {
  RunTest(REQUEST_JSON(12, 3.5), RESPONSE_JSON(3.50));
}

TEST_F(PrimitivesTest, Double) {
  RunTest(REQUEST_JSON(13, 1e13), RESPONSE_JSON(10000000000000.0));
}
