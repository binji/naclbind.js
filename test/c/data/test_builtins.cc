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
#include "glue.h"

TEST_F(GeneratorTest, Simple) {
  const int kBufferSize = 1000;
  char buffer[kBufferSize];
  const char* request_json =
      "{\"id\": 1,"
      " \"set\": {"
      "     \"1\": 4,"
      "     \"2\": 1,"
      "     \"3\": 10},"
      " \"commands\": ["
      "     {\"id\": %d, \"args\": [1], \"ret\": 4},"     // p = my_malloc(4)
      "     {\"id\": %d, \"args\": [4, 3]},"              // *p = 10
      "     {\"id\": %d, \"args\": [4], \"ret\": 5},"     // a = *p
      "     {\"id\": %d, \"args\": [5, 2], \"ret\": 6},"  // b = add(a, 1)
      "     {\"id\": %d, \"args\": [6, 1], \"ret\": 7},"  // c = sub(b, 4)
      "     {\"id\": %d, \"args\": [5, 7], \"ret\": 8},"  // d = a < c
      "     {\"id\": %d, \"args\": [4]}],"                // my_free(p)
      " \"get\": [5, 6, 7, 8],"
      " \"destroy\": [1, 2, 3, 4, 5, 6, 7, 8]}";
  snprintf(buffer,
           kBufferSize,
           request_json,
           NB_FUNC_MY_MALLOC,
           NB_FUNC_NB_SET_INT,
           NB_FUNC_NB_GET_INT,
           NB_FUNC_NB_ADD_INT,
           NB_FUNC_NB_SUB_INT,
           NB_FUNC_NB_LT_INT,
           NB_FUNC_MY_FREE);
  const char* response_json = "{\"id\":1,\"values\":[10,11,7,0]}\n";
  RunTest(buffer, response_json);
}
