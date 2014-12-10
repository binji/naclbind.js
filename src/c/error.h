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

#ifndef NB_ERROR_H_
#define NB_ERROR_H_

#include <stdio.h>

#ifndef NDEBUG
#define NB_LOG(msg) NB_ERROR(msg)
#define NB_VLOG(msg, ...) NB_VERROR(msg, __VA_ARGS__)
#else
#define NB_LOG(msg)
#define NB_VLOG(msg, ...)
#endif

#define NB_TRACE NB_LOG("enter")
#define NB_VTRACE(fmt, ...) NB_VLOG("enter" fmt, __VA_ARGS__)

#define NB_ERROR(msg) \
  fprintf(stderr, "%s:%d:(%s): " msg "\n", __FILE__, __LINE__, __FUNCTION__)

#define NB_VERROR(msg, ...)        \
  fprintf(stderr,                  \
          "%s:%d:(%s): " msg "\n", \
          __FILE__,                \
          __LINE__,                \
          __FUNCTION__,            \
          __VA_ARGS__)

#endif /* NB_ERROR_H_ */
