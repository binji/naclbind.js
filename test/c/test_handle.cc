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

#define EXPECT_GET(type, suffix, handle, expected) \
  type val_##suffix; \
  bool result_##suffix = nb_handle_get_##suffix(handle, &val_##suffix); \
  EXPECT_EQ(TRUE, result_##suffix); \
  if (result_##suffix) { EXPECT_EQ(expected, val_##suffix); }

#define EXPECT_FAIL(type, suffix, handle) \
  type val_##suffix; \
  EXPECT_EQ(FALSE, nb_handle_get_##suffix(handle, &val_##suffix))

#define EXPECT_O(type, suffix, handle, expected) \
  EXPECT_GET(type, suffix, handle, expected)

#define EXPECT_T(type, suffix, handle, expected) \
  type val_##suffix; \
  EXPECT_EQ(TRUE, nb_handle_get_##suffix(handle, &val_##suffix)); \

#define EXPECT__(type, suffix, handle, expected) \
  EXPECT_FAIL(type, suffix, handle)

#define ROW(reg, val, i8, u8, i16, u16, i32, u32, i64, u64, f32, f64, vp, v) \
  { \
    EXPECT_EQ(TRUE, nb_handle_register_##reg(1, val)); \
    EXPECT_##i8(int8_t, int8, 1, val); \
    EXPECT_##u8(uint8_t, uint8, 1, val); \
    EXPECT_##i16(int16_t, int16, 1, val); \
    EXPECT_##u16(uint16_t, uint16, 1, val); \
    EXPECT_##i32(int32_t, int32, 1, val); \
    EXPECT_##u32(uint32_t, uint32, 1, val); \
    EXPECT_##i64(int64_t, int64, 1, val); \
    EXPECT_##u64(uint64_t, uint64, 1, val); \
    EXPECT_##f32(float, float, 1, val); \
    EXPECT_##f64(double, double, 1, val); \
    EXPECT_##vp(void*, voidp, 1, val); \
    EXPECT_##v(struct PP_Var, var, 1, val); \
    nb_handle_destroy(1); \
  }

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

TEST_F(HandleTest, Int8) {
  //               i8 u8 i16 u16 i32 u32 i64 u64 f32 f64 vp  v
  ROW(int8, -0x70,  O, _,  O,  _,  O,  _,  O,  _,  O,  O, _, _);
  ROW(int8,     0,  O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int8, +0x70,  O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
}

TEST_F(HandleTest, Uint8) {
  //                i8 u8 i16 u16 i32 u32 i64 u64 f32 f64 vp  v
  ROW(uint8,     0,  O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint8, +0x70,  O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint8, +0xf0,  _, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
}

TEST_F(HandleTest, Int16) {
  //                  i8 u8 i16 u16 i32 u32 i64 u64 f32 f64 vp  v
  ROW(int16, -0x7000,  _, _,  O,  _,  O,  _,  O,  _,  O,  O, _, _);
  ROW(int16,   -0x70,  O, _,  O,  _,  O,  _,  O,  _,  O,  O, _, _);
  ROW(int16,       0,  O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int16,   +0x70,  O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int16,   +0xf0,  _, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int16, +0x7000,  _, _,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
}

TEST_F(HandleTest, Uint16) {
  //                   i8 u8 i16 u16 i32 u32 i64 u64 f32 f64 vp  v
  ROW(uint16,       0,  O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint16,   +0x70,  O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint16,   +0xf0,  _, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint16, +0x7000,  _, _,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint16, +0xf000,  _, _,  _,  O,  O,  O,  O,  O,  O,  O, _, _);
}

TEST_F(HandleTest, Int32) {
  //                      i8 u8 i16 u16 i32 u32 i64 u64 f32 f64 vp  v
  ROW(int32, -0x70000000,  _, _,  _,  _,  O,  _,  O,  _,  _,  O, _, _);
  ROW(int32,   -0x700000,  _, _,  _,  _,  O,  _,  O,  _,  O,  O, _, _);
  ROW(int32,     -0x7000,  _, _,  O,  _,  O,  _,  O,  _,  O,  O, _, _);
  ROW(int32,       -0x70,  O, _,  O,  _,  O,  _,  O,  _,  O,  O, _, _);
  ROW(int32,           0,  O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int32,       +0x70,  O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int32,       +0xf0,  _, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int32,     +0x7000,  _, _,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int32,     +0xf000,  _, _,  _,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int32,   +0x700000,  _, _,  _,  _,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int32,   +0xf00000,  _, _,  _,  _,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int32, +0x70000000,  _, _,  _,  _,  O,  O,  O,  O,  _,  O, _, _);
}

TEST_F(HandleTest, Uint32) {
  //                       i8 u8 i16 u16 i32 u32 i64 u64 f32 f64 vp  v
  ROW(uint32,           0,  O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint32,       +0x70,  O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint32,       +0xf0,  _, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint32,     +0x7000,  _, _,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint32,     +0xf000,  _, _,  _,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint32,   +0x700000,  _, _,  _,  _,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint32,   +0xf00000,  _, _,  _,  _,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint32, +0x70000000,  _, _,  _,  _,  O,  O,  O,  O,  _,  O, _, _);
  ROW(uint32, +0xf0000000,  _, _,  _,  _,  _,  O,  O,  O,  _,  O, _, _);
}

TEST_F(HandleTest, Int64) {
  //                               i8 u8 i16 u16 i32 u32 i64 u64 f32 f64 vp  v
  ROW(int64, -0x7000000000000000LL, _, _,  _,  _,  _,  _,  O,  _,  _,  _, _, _);
  ROW(int64,    -0x7000000000000LL, _, _,  _,  _,  _,  _,  O,  _,  _,  O, _, _);
  ROW(int64,           -0x70000000, _, _,  _,  _,  O,  _,  O,  _,  _,  O, _, _);
  ROW(int64,             -0x700000, _, _,  _,  _,  O,  _,  O,  _,  O,  O, _, _);
  ROW(int64,               -0x7000, _, _,  O,  _,  O,  _,  O,  _,  O,  O, _, _);
  ROW(int64,                 -0x70, O, _,  O,  _,  O,  _,  O,  _,  O,  O, _, _);
  ROW(int64,                     0, O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int64,                 +0x70, O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int64,                 +0xf0, _, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int64,               +0x7000, _, _,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int64,               +0xf000, _, _,  _,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int64,             +0x700000, _, _,  _,  _,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int64,             +0xf00000, _, _,  _,  _,  O,  O,  O,  O,  O,  O, _, _);
  ROW(int64,           +0x70000000, _, _,  _,  _,  O,  O,  O,  O,  _,  O, _, _);
  ROW(int64,           +0xf0000000, _, _,  _,  _,  _,  O,  O,  O,  _,  O, _, _);
  ROW(int64,    +0x7000000000000LL, _, _,  _,  _,  _,  _,  O,  O,  _,  O, _, _);
  ROW(int64,    +0xf000000000000LL, _, _,  _,  _,  _,  _,  O,  O,  _,  O, _, _);
  ROW(int64, +0x7000000000000000LL, _, _,  _,  _,  _,  _,  O,  O,  _,  _, _, _);
}

TEST_F(HandleTest, Uint64) {
  //                                i8 u8 i16 u16 i32 u32 i64 u64 f32 f64 vp  v
  ROW(uint64,                     0, O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint64,                 +0x70, O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint64,                 +0xf0, _, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint64,               +0x7000, _, _,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint64,               +0xf000, _, _,  _,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint64,             +0x700000, _, _,  _,  _,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint64,             +0xf00000, _, _,  _,  _,  O,  O,  O,  O,  O,  O, _, _);
  ROW(uint64,           +0x70000000, _, _,  _,  _,  O,  O,  O,  O,  _,  O, _, _);
  ROW(uint64,           +0xf0000000, _, _,  _,  _,  _,  O,  O,  O,  _,  O, _, _);
  ROW(uint64,    +0x7000000000000LL, _, _,  _,  _,  _,  _,  O,  O,  _,  O, _, _);
  ROW(uint64,    +0xf000000000000LL, _, _,  _,  _,  _,  _,  O,  O,  _,  O, _, _);
  ROW(uint64, +0x7000000000000000LL, _, _,  _,  _,  _,  _,  O,  O,  _,  _, _, _);
  ROW(uint64, +0xf000000000000000LL, _, _,  _,  _,  _,  _,  _,  O,  _,  _, _, _);
}

TEST_F(HandleTest, Float) {
  //               i8 u8 i16 u16 i32 u32 i64 u64 f32 f64 vp  v
  ROW(float,  0.f,  O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(float,  1.f,  O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(float, 3.5f,  _, _,  _,  _,  _,  _,  _,  _,  O,  O, _, _);
}

TEST_F(HandleTest, Double) {
  //                i8 u8 i16 u16 i32 u32 i64 u64 f32 f64 vp  v
  ROW(double,  0.0,  O, O,  O,  O,  O,  O,  O,  O,  O,  O, _, _);
  ROW(double, 1e11,  _, _,  _,  _,  _,  _,  O,  O,  T,  O, _, _);
  ROW(double, 1e20,  _, _,  _,  _,  _,  _,  _,  _,  T,  O, _, _);
}
