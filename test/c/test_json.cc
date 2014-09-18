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
#include <json/reader.h>
#include <ppapi/c/pp_var.h>
#include "fake_interfaces.h"
#include "json.h"
#include "message.h"
#include "var.h"

class JsonTest : public ::testing::Test {
 public:
  void RoundTrip(const char* json, const char* expected) {
    struct PP_Var var = json_to_var(json);
    char* s = var_to_json_flat(var);
    nb_var_release(var);

    EXPECT_STREQ(expected, s);
    free(s);
  }

  virtual void TearDown() {
    EXPECT_EQ(TRUE, fake_var_check_no_references());
  }
 private:
};

TEST_F(JsonTest, Null) {
  RoundTrip("null", "null\n");
}

TEST_F(JsonTest, Int) {
  RoundTrip("42", "42\n");
}

TEST_F(JsonTest, Double) {
  RoundTrip("3.5", "3.50\n");
}

TEST_F(JsonTest, String) {
  RoundTrip("\"hello, world\"", "\"hello, world\"\n");
}

TEST_F(JsonTest, Bool) {
  RoundTrip("true", "true\n");
}

TEST_F(JsonTest, Array) {
  RoundTrip("[true, 1, 2.0, \"hi\"]", "[true,1,2.0,\"hi\"]\n");
}

TEST_F(JsonTest, Dictionary) {
  RoundTrip("{\"hi\": 12, \"bye\": 23}", "{\"bye\":23,\"hi\":12}\n");
}

TEST_F(JsonTest, Complex) {
  RoundTrip("[[1, 2], {\"foo\": [3, 4]}, [[null]]]",
            "[[1,2],{\"foo\":[3,4]},[[null]]]\n");
}
