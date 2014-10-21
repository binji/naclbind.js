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

TEST_F(GeneratorTest, MultipleCommands) {
  const char *request_json =
    "{\"id\": 1,"
    " \"set\": {\"1\": \"Hello\","
    "           \"2\": 6,"
    "           \"3\": 5},"
    " \"commands\": [{\"id\": 0, \"args\": [2], \"ret\": 4},"  // malloc
    "                {\"id\": 1, \"args\": [4, 1, 2]},"        // memcpy
    "                {\"id\": 3, \"args\": [4, 3]},"           // rot13
    "                {\"id\": 4, \"args\": [4], \"ret\": 5},"  // char_to_var
    "                {\"id\": 5, \"args\": [5]},"              // var_release
    "                {\"id\": 2, \"args\": [4]}],"             // free
    " \"get\": [5],"
    " \"destroy\": [1, 2, 3, 4, 5]}";
  const char* response_json = "{\"id\":1,\"values\":[\"Uryyb\"]}\n";
  RunTest(request_json, response_json);
}
