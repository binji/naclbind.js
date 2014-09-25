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
#include <ppapi_simple/ps.h>
#include <ppapi_simple/ps_main.h>
#include "fake_interfaces.h"
#include "interfaces.h"

int nacl_main(int argc, char* argv[]) {
  PP_Instance instance = 1;
  setenv("TERM", "xterm-256color", 0);
  fake_var_init();
  nb_interfaces_init(instance, fake_get_browser_interface);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

PPAPI_SIMPLE_REGISTER_MAIN(nacl_main);
