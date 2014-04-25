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

#ifndef ZIP_TYPE_H_
#define ZIP_TYPE_H_

#include "type.h"

enum {
  TYPE_TM_ZIP_S = NUM_BUILTIN_TYPES,
  TYPE_ZIP_FILEINFO,
  TYPE_ZIP_FILEINFO_P,
  TYPE_ZIPFILE,
  TYPE_ZIPFILE_P,

  TYPE_FUNC_ZIP_OPEN,
  TYPE_FUNC_ZIP_OPEN_NEW_FILE_IN_ZIP,
  TYPE_FUNC_ZIP_WRITE_IN_FILE_IN_ZIP,
  TYPE_FUNC_ZIP_CLOSE_FILE_IN_ZIP,
  TYPE_FUNC_ZIP_CLOSE,

  TYPE_STAT,
  TYPE_STAT_P,
  TYPE_FILE,
  TYPE_FILE_P,

  TYPE_FUNC_FOPEN,
  TYPE_FUNC_FREAD,
  TYPE_FUNC_FCLOSE,
  TYPE_FUNC_STAT,
  NUM_TYPES
};

const char* TypeToString(Type id);

#endif  // ZIP_TYPE_H_

