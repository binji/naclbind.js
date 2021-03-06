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

/* DO NOT EDIT, this file is auto-generated from //templates/glue.h */

#ifndef NB_{{module_name.upper()}}_GLUE_H_
#define NB_{{module_name.upper()}}_GLUE_H_

enum {
  NB_FUNC_ERROR_IF = 0,
[[for fn in collector.functions:]]
  NB_FUNC_{{fn.spelling.upper()}} = {{fn.fn_id}},
[[]]
};

#endif /* NB_{{module_name.upper()}}_GLUE_H_ */
