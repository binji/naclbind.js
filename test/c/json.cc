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

#include "json.h"
#include <string.h>
#include <json/reader.h>
#include <json/writer.h>
#include "error.h"
#include "var.h"

using namespace Json;

static PP_Var value_to_var(const Value& value) {
  switch (value.type()) {
    case nullValue:
      return PP_MakeNull();

    case intValue:
      return PP_MakeInt32(value.asInt());

    case uintValue:
      return PP_MakeInt32(value.asInt());

    case realValue:
      return PP_MakeDouble(value.asDouble());

    case stringValue: {
      std::string s = value.asString();
      return nb_var_string_create(s.c_str(), s.size());
    }

    case booleanValue:
      return PP_MakeBool(value.asBool() ? PP_TRUE : PP_FALSE);

    case arrayValue: {
      struct PP_Var array = nb_var_array_create();
      for (ArrayIndex i = 0; i < value.size(); ++i) {
        struct PP_Var child = value_to_var(value[i]);
        if (child.type == PP_VARTYPE_UNDEFINED) {
          nb_var_release(array);
          return PP_MakeUndefined();
        }

        if (!nb_var_array_set(array, i, child)) {
          nb_var_release(child);
          nb_var_release(array);
          return PP_MakeUndefined();
        }

        nb_var_release(child);
      }
      return array;
    }

    case objectValue: {
      struct PP_Var dict = nb_var_dict_create();
      Value::Members members = value.getMemberNames();
      for (Value::Members::const_iterator iter = members.begin();
           iter != members.end();
           ++iter) {
        std::string key = *iter;
        Value child_value = value[key];
        struct PP_Var child_value_var = value_to_var(child_value);
        if (!nb_var_dict_set(dict, key.c_str(), child_value_var)) {
          nb_var_release(child_value_var);
          nb_var_release(dict);
          return PP_MakeUndefined();
        }

        nb_var_release(child_value_var);
      }
      return dict;
    }

    default:
      return PP_MakeUndefined();
  }
}

static Value var_to_value(struct PP_Var var) {
  switch (var.type) {
    case PP_VARTYPE_NULL:
      return Value();

    case PP_VARTYPE_BOOL:
      return Value(var.value.as_bool ? true : false);

    case PP_VARTYPE_INT32:
      return Value(var.value.as_int);

    case PP_VARTYPE_DOUBLE:
      return Value(var.value.as_double);

    case PP_VARTYPE_STRING: {
      const char* s;
      uint32_t len;
      if (!nb_var_string(var, &s, &len)) {
        return Value();
      }
      return Value(std::string(s, len));
    }

    case PP_VARTYPE_ARRAY: {
      Value value(arrayValue);
      uint32_t len = nb_var_array_length(var);

      for (uint32_t i = 0; i < len; ++i) {
        struct PP_Var child = nb_var_array_get(var, i);
        value.append(var_to_value(child));
        nb_var_release(child);
      }

      return value;
    }

    case PP_VARTYPE_DICTIONARY: {
      Value value(objectValue);
      struct PP_Var keys = nb_var_dict_get_keys(var);
      uint32_t len = nb_var_array_length(keys);
      for (uint32_t i = 0; i < len; ++i) {
        struct PP_Var key = nb_var_array_get(keys, i);
        const char* key_data;
        uint32_t key_len;

        if (!nb_var_string(key, &key_data, &key_len)) {
          nb_var_release(key);
          nb_var_release(keys);
          return Value();
        }

        nb_var_release(key);

        std::string key_string(key_data, key_len);
        struct PP_Var child = nb_var_dict_get_var(var, key);
        value[key_string] = var_to_value(child);

        nb_var_release(child);
      }

      nb_var_release(keys);
      return value;
    }

    default:
      NB_VERROR("Unexpected type %s", nb_var_type_to_string(var.type));
      return Value();
  }
}

struct PP_Var json_to_var(const char* string) {
  Reader reader;
  Value value;
  if (!reader.parse(string, value, false)) {
    NB_VERROR("Error parsing json:\n%s",
              reader.getFormattedErrorMessages().c_str());
    return PP_MakeUndefined();
  }

  return value_to_var(value);
}

char* var_to_json(struct PP_Var var) {
  StyledWriter writer;
  std::string s = writer.write(var_to_value(var));
  return strdup(s.c_str());
}

char* var_to_json_flat(struct PP_Var var) {
  FastWriter writer;
  std::string s = writer.write(var_to_value(var));
  return strdup(s.c_str());
}
