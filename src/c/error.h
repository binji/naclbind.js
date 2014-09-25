/* Copyright 2014 Ben Smith. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ERROR_H_
#define ERROR_H_

#include <stdio.h>

#ifndef NDEBUG
#define LOG(msg) ERROR(msg)
#define VLOG(msg, ...) VERROR(msg, __VA_ARGS__)
#else
#define LOG(msg)
#define VLOG(msg, ...)
#endif

#define TRACE LOG("enter")
#define VTRACE(fmt, ...) VLOG("enter" fmt, __VA_ARGS__)

#define ERROR(msg) \
  fprintf(stderr, "%s:%d:(%s): " msg "\n", __FILE__, __LINE__, __FUNCTION__)

#define VERROR(msg, ...) \
  fprintf(stderr, "%s:%d:(%s): " msg "\n", __FILE__, __LINE__, __FUNCTION__, \
          __VA_ARGS__)

#define ERROR_IF(cond, msg) \
  if (!(cond)) { ERROR(msg); return; }

#define VERROR_IF(cond, msg, ...) \
  if (!(cond)) { VERROR(msg, __VA_ARGS__); return; }

#endif  /* ERROR_H_ */
