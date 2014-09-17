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

#include "json.h"
#include "message.h"
#include "var.h"

TEST(Json, Null) {
  struct PP_Var var = json_to_var("null");
  char* s = var_to_json_flat(var);
  EXPECT_STREQ(s, "null\n");
  free(s);
  nb_var_release(var);
}

TEST(Json, Int) {
  struct PP_Var var = json_to_var("42");
  char* s = var_to_json_flat(var);
  EXPECT_STREQ(s, "42\n");
  free(s);
  nb_var_release(var);
}

TEST(Json, Double) {
  struct PP_Var var = json_to_var("3.5");
  char* s = var_to_json_flat(var);
  EXPECT_STREQ(s, "3.50\n");
  free(s);
  nb_var_release(var);
}

TEST(Json, String) {
  struct PP_Var var = json_to_var("\"hello, world\"");
  char* s = var_to_json_flat(var);
  EXPECT_STREQ(s, "\"hello, world\"\n");
  free(s);
  nb_var_release(var);
}

TEST(Json, Bool) {
  struct PP_Var var = json_to_var("true");
  char* s = var_to_json_flat(var);
  EXPECT_STREQ(s, "true\n");
  free(s);
  nb_var_release(var);
}

TEST(Json, Array) {
  struct PP_Var var = json_to_var("[true, 1, 2.0, \"hi\"]");
  char* s = var_to_json_flat(var);
  EXPECT_STREQ(s, "[true,1,2.0,\"hi\"]\n");
  free(s);
  nb_var_release(var);
}

TEST(Json, Dictionary) {
  struct PP_Var var = json_to_var("{\"hi\": 12, \"bye\": 23}");
  char* s = var_to_json_flat(var);
  EXPECT_STREQ(s, "{\"bye\":23,\"hi\":12}\n");
  free(s);
  nb_var_release(var);
}

TEST(Json, Complex) {
  struct PP_Var var = json_to_var("[[1, 2], {\"foo\": [3, 4]}, [[null]]]");
  char* s = var_to_json_flat(var);
  EXPECT_STREQ(s, "[[1,2],{\"foo\":[3,4]},[[null]]]\n");
  free(s);
  nb_var_release(var);
}
