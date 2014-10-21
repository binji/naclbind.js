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

TEST_F(GeneratorTest, FunctionPointers) {
  const char *request_json =
    "{\"id\": 1,"
    " \"set\": {\"1\": 1},"
    " \"commands\": ["
    "     {\"id\": -1, \"args\": [1], \"ret\": 2},"  // get_func(twice)
    "     {\"id\": 2, \"args\": [2], \"ret\": 3}],"  // do_42
    " \"get\": [3],"
    " \"destroy\": [1, 2, 3]}";
  const char* response_json = "{\"id\":1,\"values\":[84]}\n";
  RunTest(request_json, response_json);
}