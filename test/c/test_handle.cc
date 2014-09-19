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
#include "handle.h"
#include "var.h"

class HandleTest : public ::testing::Test {
 public:
  HandleTest() {}

  virtual void SetUp() {
    EXPECT_EQ(0, nb_handle_count());
  }

  virtual void TearDown() {
    EXPECT_EQ(TRUE, fake_var_check_no_references());
    EXPECT_EQ(0, nb_handle_count());
  }
};

TEST_F(HandleTest, Basic) {
  struct PP_Var var;
  void* voidp = &var;

  var = nb_var_string_create("hello", 5);

  EXPECT_EQ(TRUE, nb_handle_register_int8(1, -42));
  EXPECT_EQ(TRUE, nb_handle_register_uint8(2, 42));
  EXPECT_EQ(TRUE, nb_handle_register_int16(3, -420));
  EXPECT_EQ(TRUE, nb_handle_register_uint16(4, 420));
  EXPECT_EQ(TRUE, nb_handle_register_int32(5, -420000L));
  EXPECT_EQ(TRUE, nb_handle_register_uint32(6, 420000UL));
  EXPECT_EQ(TRUE, nb_handle_register_int64(7, -42000000000LL));
  EXPECT_EQ(TRUE, nb_handle_register_uint64(8, 42000000000ULL));
  EXPECT_EQ(TRUE, nb_handle_register_float(9, 3.25));
  EXPECT_EQ(TRUE, nb_handle_register_double(10, 1e30));
  EXPECT_EQ(TRUE, nb_handle_register_voidp(11, voidp));
  EXPECT_EQ(TRUE, nb_handle_register_var(12, var));

#define EXPECT_GET(type, suffix, handle, expected) \
  type val_##suffix; \
  EXPECT_EQ(TRUE, nb_handle_get_##suffix(handle, &val_##suffix)); \
  EXPECT_EQ(expected, val_##suffix)

  EXPECT_GET(int8_t, int8, 1, -42);
  EXPECT_GET(uint8_t, uint8, 2, 42);
  EXPECT_GET(int16_t, int16, 3, -420);
  EXPECT_GET(uint16_t, uint16, 4, 420);
  EXPECT_GET(int32_t, int32, 5, -420000L);
  EXPECT_GET(uint32_t, uint32, 6, 420000UL);
  EXPECT_GET(int64_t, int64, 7, -42000000000LL);
  EXPECT_GET(uint64_t, uint64, 8, 42000000000ULL);
  EXPECT_GET(float, float, 9, 3.25);
  EXPECT_GET(double, double, 10, 1e30);
  EXPECT_GET(void*, voidp, 11, voidp);

  struct PP_Var val_var;
  EXPECT_EQ(TRUE, nb_handle_get_var(12, &val_var));
  EXPECT_EQ(PP_VARTYPE_STRING, val_var.type);
  EXPECT_EQ(val_var.value.as_id, var.value.as_id);

  nb_var_release(var);

  Handle to_destroy[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
  nb_handle_destroy_many(&to_destroy[0], 12);
}
