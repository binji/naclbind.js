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

#define REQUEST3(fmt, h1, h2, h3)                                       \
    "{\"id\": 1,"                                                       \
    " \"set\": {\"1\": 42,"                                             \
    "           \"2\": 3.5,"                                            \
    "           \"3\": 120,"                                            \
    "           \"4\": \""fmt"\"},"                                     \
    " \"commands\": ["                                                  \
    "     {\"id\": 2, \"args\": [3], \"ret\": 5},"                      \
    "     {\"id\": 1, \"args\": [4, "#h1", "#h2", "#h3"], \"ret\": 6}," \
    "     {\"id\": 3, \"args\": [5]}],"                                 \
    " \"get\": [6],"                                                    \
    " \"destroy\": [1, 2, 3, 4, 5, 6]"                                  \
    "}"

#define RESPONSE3 "{\"id\":1,\"values\":[165.50]}\n"

TEST_F(GeneratorTest, VariadicIDP) {
  const char* request = REQUEST3("idp", 1, 2, 5);
  RunTest(request, RESPONSE3);
}

TEST_F(GeneratorTest, VariadicIPD) {
  const char* request = REQUEST3("ipd", 1, 5, 2);
  RunTest(request, RESPONSE3);
}

TEST_F(GeneratorTest, VariadicDIP) {
  const char* request = REQUEST3("dip", 2, 1, 5);
  RunTest(request, RESPONSE3);
}

TEST_F(GeneratorTest, VariadicDPI) {
  const char* request = REQUEST3("dpi", 2, 5, 1);
  RunTest(request, RESPONSE3);
}

TEST_F(GeneratorTest, VariadicPDI) {
  const char* request = REQUEST3("pdi", 5, 2, 1);
  RunTest(request, RESPONSE3);
}

TEST_F(GeneratorTest, VariadicPID) {
  const char* request = REQUEST3("pid", 5, 1, 2);
  RunTest(request, RESPONSE3);
}
