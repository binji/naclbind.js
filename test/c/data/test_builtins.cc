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

// TODO(binji): better way to get function ids?
// add/sub
// x + 0: (void*, int)
// x + 1: (int, int)
// x + 2: (uint, uint)
// x + 3: (longlong, longlong)
// x + 4: (ulonglong, ulonglong)
// x + 5: (float, float)
// x + 6: (double, double)
// add 29..35
// sub 36..42

// get/set/eq/ne/lt/gt/le/ge
// x + 0: (void*, void*)
// x + 1: (char, char)
// x + 2: (schar, schar)
// x + 3: (uchar, uchar)
// x + 4: (short, short)
// x + 5: (ushort, ushort)
// x + 6: (ushort, ushort)
// x + 7: (int, int)
// x + 8: (uint, uint)
// x + 9: (long, long)
// x + 10: (ulong, ulong)
// x + 11: (longlong, longlong)
// x + 12: (ulonglong, ulonglong)
// x + 13: (float, float)
// x + 14: (double, double)
// get 1..14
// set 15..28
// lt 43..56
// le 57..70
// gt 71..84
// ge 85..98
// eq 99..112
// ne 113..126
//
// my_malloc 127
// my_free 128

TEST_F(GeneratorTest, Simple) {
  const char* request_json =
      "{\"id\": 1,"
      " \"set\": {"
      "     \"1\": 4,"
      "     \"2\": 1,"
      "     \"3\": 10},"
      " \"commands\": ["
      "     {\"id\": 127, \"args\": [1], \"ret\": 4},"    // p = my_malloc(4)
      "     {\"id\": 22, \"args\": [4, 3]},"              // *p = 10
      "     {\"id\": 8, \"args\": [4], \"ret\": 5},"      // a = *p
      "     {\"id\": 30, \"args\": [5, 2], \"ret\": 6},"  // b = add(a, 1)
      "     {\"id\": 37, \"args\": [6, 1], \"ret\": 7},"  // c = sub(b, 4)
      "     {\"id\": 50, \"args\": [5, 7], \"ret\": 8},"  // d = a < c
      "     {\"id\": 128, \"args\": [4]}],"               // my_free(p)
      " \"get\": [5, 6, 7, 8],"
      " \"destroy\": [1, 2, 3, 4, 5, 6, 7, 8]}";
  const char* response_json = "{\"id\":1,\"values\":[10,11,7,0]}\n";
  RunTest(request_json, response_json);
}
