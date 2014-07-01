# Copyright 2014 Ben Smith. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import collections

# See http://stackoverflow.com/a/14620633
class AttrDict(dict):
  def __init__(self, *args, **kwargs):
    super(AttrDict, self).__init__(*args, **kwargs)
    self.__dict__ = self


FIRST_ID = 1000

BUILTIN_TYPES = [
  {'name': 'void', 'kind': 'void'},
  {'name': 'char', 'kind': 'prim', 'size': 1, 'is_signed': True, 'is_int': True},
  {'name': 'int8', 'kind': 'prim', 'size': 1, 'is_signed': True, 'is_int': True},
  {'name': 'uint8', 'kind': 'prim', 'size': 1, 'is_signed': False, 'is_int': True},
  {'name': 'int16', 'kind': 'prim', 'size': 2, 'is_signed': True, 'is_int': True},
  {'name': 'uint16', 'kind': 'prim', 'size': 2, 'is_signed': False, 'is_int': True},
  {'name': 'int32', 'kind': 'prim', 'size': 4, 'is_signed': True, 'is_int': True},
  {'name': 'uint32', 'kind': 'prim', 'size': 4, 'is_signed': False, 'is_int': True},
  {'name': 'long', 'kind': 'prim', 'size': 4, 'is_signed': True, 'is_int': True},
  {'name': 'ulong', 'kind': 'prim', 'size': 4, 'is_signed': False, 'is_int': True},
  {'name': 'int64', 'kind': 'prim', 'size': 8, 'is_signed': True, 'is_int': True},
  {'name': 'uint64', 'kind': 'prim', 'size': 8, 'is_signed': False, 'is_int': True},
  {'name': 'float32', 'kind': 'prim', 'size': 4, 'is_int': False},
  {'name': 'float64', 'kind': 'prim', 'size': 8, 'is_int': False},

  {'name': 'uchar', 'kind': 'alias', 'alias': 'uint8', 'str': 'unsigned char'},
  {'name': 'short', 'kind': 'alias', 'alias': 'int16', 'str': 'short'},
  {'name': 'ushort', 'kind': 'alias', 'alias': 'uint16', 'str': 'unsigned short'},
  {'name': 'int', 'kind': 'alias', 'alias': 'int32', 'str': 'int'},
  {'name': 'uint', 'kind': 'alias', 'alias': 'uint32', 'str': 'unsigned int'},
  {'name': 'longlong', 'kind': 'alias', 'alias': 'int64', 'str': 'long long'},
  {'name': 'ulonglong', 'kind': 'alias', 'alias': 'uint64', 'str': 'unsigned long long'},
  {'name': 'float', 'kind': 'alias', 'alias': 'float32'},
  {'name': 'double', 'kind': 'alias', 'alias': 'float64'},
  {'name': 'size_t', 'kind': 'alias', 'alias': 'uint32'},
  {'name': 'ssize_t', 'kind': 'alias', 'alias': 'int32'},
  {'name': 'off_t', 'kind': 'alias', 'alias': 'int64'},

  {'name': 'void$', 'kind': 'pointer', 'base': 'void'},
  {'name': 'char$', 'kind': 'pointer', 'base': 'char'},
  {'name': 'int8$', 'kind': 'pointer', 'base': 'int8'},
  {'name': 'uint8$', 'kind': 'pointer', 'base': 'uint8'},
  {'name': 'int16$', 'kind': 'pointer', 'base': 'int16'},
  {'name': 'uint16$', 'kind': 'pointer', 'base': 'uint16'},
  {'name': 'int32$', 'kind': 'pointer', 'base': 'int32'},
  {'name': 'uint32$', 'kind': 'pointer', 'base': 'uint32'},
  {'name': 'long$', 'kind': 'pointer', 'base': 'long'},
  {'name': 'ulong$', 'kind': 'pointer', 'base': 'ulong'},
  {'name': 'int64$', 'kind': 'pointer', 'base': 'int64'},
  {'name': 'uint64$', 'kind': 'pointer', 'base': 'uint64'},
  {'name': 'float32$', 'kind': 'pointer', 'base': 'float32'},
  {'name': 'float64$', 'kind': 'pointer', 'base': 'float64'},
  {'name': 'void$$', 'kind': 'pointer', 'base': 'void$'},

  {'name': 'Var', 'kind': 'pepper', 'pepper_type': 'Var'},
  {'name': 'ArrayBuffer', 'kind': 'pepper', 'pepper_type': 'ArrayBuffer'},
  {'name': 'Array', 'kind': 'pepper', 'pepper_type': 'Array'},
  {'name': 'Dictionary', 'kind': 'pepper', 'pepper_type': 'Dictionary'},
  {'name': 'String', 'kind': 'pepper', 'pepper_type': 'String'},
]
# Convert each element to AttrDict
BUILTIN_TYPES = [AttrDict(d) for d in BUILTIN_TYPES]

def CamelToSnake(name):
  result = ''
  was_lower = False
  for c in name:
    if was_lower and c.isupper():
      result += '_' + c
    else:
      result += c
    was_lower = c.islower()
  return result


def CamelToMacro(name):
  return CamelToSnake(name).upper()


def Titlecase(name):
  return name[0].upper() + name[1:]


def CommaSep(arr):
  return ', '.join(arr)


def MakeCIdent(js_ident):
  return js_ident.replace('$', '_p')


class IdGenerator(object):
  def __init__(self):
    self.next_id = 1

  def Get(self):
    id_ = self.next_id
    self.next_id += 1
    return id_

  def Set(self, next_id):
    self.next_id = next_id


class TypeData(object):
  KIND_VOID = 0
  KIND_POINTER = 1
  KIND_PRIMITIVE = 2
  KIND_PEPPER = 3
  KIND_STRUCT = 4
  KIND_FUNCTION = 5

  def GetFormat(self):
    return None

  def GetFormatArg(self, name):
    return None

  def GetArgString(self, name):
    return self.GetFormatArg(name)

  @property
  def is_void(self):
    return self.kind == self.KIND_VOID

  @property
  def is_pointer(self):
    return self.kind == self.KIND_POINTER

  @property
  def is_primitive(self):
    return self.kind == self.KIND_PRIMITIVE

  @property
  def is_pepper(self):
    return self.kind == self.KIND_PEPPER

  @property
  def is_struct(self):
    return self.kind == self.KIND_STRUCT

  @property
  def is_function(self):
    return self.kind == self.KIND_FUNCTION

  def __ne__(self, other):
    return not (self == other)


class VoidTypeData(TypeData):
  kind = TypeData.KIND_VOID

  def __str__(self):
    return 'void'

  def __eq__(self, other):
    return isinstance(other, TypeData) and other.is_void

  def __hash__(self):
    return hash(VoidTypeData)


class PrimitiveTypeData(TypeData):
  kind = TypeData.KIND_PRIMITIVE

  TO_STR = {
    "char": "char",
    "int8": "int8_t",
    "uint8": "uint8_t",
    "int16": "int16_t",
    "uint16": "uint16_t",
    "int32": "int32_t",
    "uint32": "uint32_t",
    "long": "long",
    "ulong": "unsigned long",
    "int64": "int64_t",
    "uint64": "uint64_t",
    "float32": "float",
    "float64": "double",
  }

  def __init__(self, js_ident, is_signed, size, is_int):
    TypeData.__init__(self)
    self.js_ident = js_ident
    self.is_signed = is_signed
    self.size = size
    self.is_int = is_int
    self.c_str = self.TO_STR[js_ident]
    self.is_long = 'long' in self.js_ident

  def GetFormat(self):
    if self.is_int:
      if self.size <= 4:
        if self.size == 4 and self.is_long:
          return '%ld' if self.is_signed else '%lu'
        else:
          return '%d' if self.is_signed else '%u'
      else:
        return '%lld' if self.is_signed else '%llu'
    else:
      return '%g'

  def GetFormatArg(self, name):
    return name

  def __str__(self):
    return self.c_str

  def __eq__(self, other):
    return (isinstance(other, TypeData) and
            other.is_primitive and
            self.js_ident == other.js_ident and
            self.is_signed == other.is_signed and
            self.size == other.size and
            self.is_int == other.is_int)

  def __hash__(self):
    return hash((PrimitiveTypeData, self.js_ident, self.is_signed, self.size,
                 self.is_int))


class StructField(object):
  def __init__(self, name, type_, offset):
    self.name = name
    self.offset = offset
    self.type = type_


class StructTypeData(TypeData):
  kind = TypeData.KIND_STRUCT

  def __init__(self, name, size, fields):
    TypeData.__init__(self)
    self.name = name
    self.size = size
    self.fields = fields

  def __str__(self):
    return 'struct %s' % self.name

  def __eq__(self, other):
    return (isinstance(other, TypeData) and
            other.is_struct and
            self.name == other.name)

  def __hash__(self):
    return hash((StructTypeData, self.name))


class PepperTypeData(TypeData):
  kind = TypeData.KIND_PEPPER
  TYPE_VAR = 0
  TYPE_STRING = 1
  TYPE_ARRAY = 2
  TYPE_ARRAY_BUFFER = 3
  TYPE_DICTIONARY = 4
  TO_JS_PROTOTYPE = {
    TYPE_VAR: "undefined",
    TYPE_STRING: "String",
    TYPE_ARRAY: "Array",
    TYPE_ARRAY_BUFFER: "ArrayBuffer",
    TYPE_DICTIONARY: "Object"
  }

  def __init__(self, pepper_type):
    TypeData.__init__(self)
    self.pepper_type = pepper_type
    self.js_prototype = self.TO_JS_PROTOTYPE[pepper_type]

  def GetFormat(self):
    return '%s'

  def GetFormatArg(self, name):
    return 'VarTypeToString(&%s)' % name

  def GetArgString(self, name):
    return name

  def __str__(self):
    return 'struct PP_Var'

  def __eq__(self, other):
    return (isinstance(other, TypeData) and
            other.is_pepper and
            self.pepper_type == other.pepper_type)

  def __hash__(self):
    return hash((PepperTypeData, self.pepper_type))


class PointerTypeData(TypeData):
  kind = TypeData.KIND_POINTER

  def __init__(self, base_type):
    TypeData.__init__(self)
    self.base_type = base_type
    self.c_str = str(self.base_type) + '*'

  def GetFormat(self):
    return '%p'

  def GetFormatArg(self, name):
    return name

  def __str__(self):
    return self.c_str

  def __eq__(self, other):
    return (isinstance(other, TypeData) and
            other.is_pointer and
            self.base_type.data == other.base_type.data)

  def __hash__(self):
    return hash((PointerTypeData, self.base_type.data))


class FunctionTypeData(TypeData):
  kind = TypeData.KIND_FUNCTION

  def __init__(self, return_type, arg_types):
    TypeData.__init__(self)
    self.return_type = return_type
    self.arg_types = arg_types

    args = ', '.join(map(str, self.arg_types))
    self.c_str = '%s (*)(%s)' % (self.return_type, args)

  def _GetArgTypeData(self):
    return [arg_type.data for arg_type in self.arg_types]

  def __str__(self):
    return self.c_str

  def __eq__(self, other):
    return (isinstance(other, TypeData) and
            other.is_function and
            self._GetArgTypeData() == other._GetArgTypeData() and
            self.return_type.data == other.return_type.data)

  def __hash__(self):
    return hash((FunctionTypeData,
                 self.return_type.data,
                 tuple(self._GetArgTypeData())))


class Type(object):
  def __init__(self, id_, js_ident, c_ident, c_str, type_data,
               is_builtin=False, alias_of=None):
    assert isinstance(type_data, TypeData)
    assert alias_of is None or isinstance(alias_of, Type)
    self.id = id_
    self.js_ident = js_ident
    self.c_ident = c_ident
    self.c_str = c_str
    self.data = type_data
    self.is_builtin = is_builtin
    self.alias_of = alias_of

  @property
  def is_alias(self):
    return self.alias_of is not None

  def __str__(self):
    return self.c_str

  def __getattr__(self, name):
    return getattr(self.data, name)


class Types(object):
  def __init__(self):
    self.type_ident_dict = {}
    self.no_builtins = collections.OrderedDict()
    self.function_types = collections.OrderedDict()
    self.type_data_dict = collections.defaultdict(list)

  def AddType(self, type_):
    self.type_data_dict[type_.data].append(type_)

    if type_.data.is_function:
      self.function_types[type_.js_ident] = type_
    else:
      # TODO(binji): handle struct namespace; i.e. struct foo != typedef foo
      assert type_.js_ident not in self.type_ident_dict, \
          'typename "%s" already in dict' % type_.js_ident
      self.type_ident_dict[type_.js_ident] = type_

      if not type_.is_builtin:
        self.no_builtins[type_.js_ident] = type_


class TypesBuilder(object):
  def __init__(self, add_builtin_types):
    self.types = Types()
    self.id_gen = IdGenerator()
    if add_builtin_types:
      self.AddTypeDicts(BUILTIN_TYPES, is_builtin=True)
      self.id_gen.Set(FIRST_ID)

  def AddTypeDicts(self, type_dicts, is_builtin=False):
    for type_dict in type_dicts:
      self.AddTypeDict(type_dict, is_builtin)

  def AddTypeDict(self, type_dict, is_builtin=False):
    type_ = self._TypeDictToType(type_dict, is_builtin)
    self.types.AddType(type_)
    return type_

  def AddFunctionDict(self, fn_dict, is_builtin):
    if fn_dict.get('overloads', False):
      fn_dicts = fn_dict.overloads
    else:
      fn_dicts = [fn_dict]

    types = []
    for fn_dict in fn_dicts:
      type_ = self._FunctionDictToType(fn_dict, is_builtin)
      self.types.AddType(type_)
      types.append(type_)
    return types

  def _TypeDictToType(self, type_dict, is_builtin):
    type_data = self._TypeDictToTypeData(type_dict)
    kind = type_dict.kind
    return self._DictToType(type_dict, kind, type_data, is_builtin)

  def _FunctionDictToType(self, fn_dict, is_builtin):
    type_data = self._FunctionDictToTypeData(fn_dict)
    return self._DictToType(fn_dict, None, type_data, is_builtin)

  def _TypeDictToTypeData(self, type_dict):
    if type_dict.kind == 'void':
      return VoidTypeData()
    elif type_dict.kind == 'prim':
      is_signed = getattr(type_dict, 'is_signed', False)
      return PrimitiveTypeData(type_dict.name, is_signed, type_dict.size,
                               type_dict.is_int)
    elif type_dict.kind == 'struct':
      fields = []
      for field in type_dict.fields:
        field_type = self.types.type_ident_dict[field.type]
        fields.append(StructField(field.name, field_type, field.offset))
      return StructTypeData(type_dict.name, type_dict.size, fields)
    elif type_dict.kind == 'pepper':
      if type_dict.pepper_type == 'Var':
        pepper_type = PepperTypeData.TYPE_VAR
      elif type_dict.pepper_type == 'String':
        pepper_type = PepperTypeData.TYPE_STRING
      elif type_dict.pepper_type == 'Array':
        pepper_type = PepperTypeData.TYPE_ARRAY
      elif type_dict.pepper_type == 'ArrayBuffer':
        pepper_type = PepperTypeData.TYPE_ARRAY_BUFFER
      elif type_dict.pepper_type == 'Dictionary':
        pepper_type = PepperTypeData.TYPE_DICTIONARY
      else:
        raise Exception('Unknown pepper_type %r' % type_dict.pepper_type)
      return PepperTypeData(pepper_type)
    elif type_dict.kind == 'pointer':
      return PointerTypeData(self.types.type_ident_dict[type_dict.base])
    elif type_dict.kind == 'function':
      return self.FunctionDictToTypeData(type_dict)
    elif type_dict.kind == 'alias':
      # Handled in _DictToType. That way we can specify the correct alias even
      # if there are multiple with the same TypeData.
      return None
    raise Exception('Unknown kind %r' % type_dict.kind)

  def _FunctionDictToTypeData(self, fn_dict):
    return_type = self.types.type_ident_dict[fn_dict.result]
    arg_types = []
    for arg_type_ident in fn_dict.args:
      arg_types.append(self.types.type_ident_dict[arg_type_ident])
    return FunctionTypeData(return_type, arg_types)

  def _DictToType(self, dict_, kind, type_data, is_builtin):
    other_types = self.types.type_data_dict.get(type_data)
    alias_of = None
    js_ident = dict_.name
    c_ident = MakeCIdent(js_ident)
    c_str = getattr(dict_, 'str', None)
    if not c_str:
      if type_data:
        c_str = str(type_data)
      else:
        c_str = c_ident

    if kind == 'alias':
      alias_of = self.types.type_ident_dict[dict_.alias]
    elif other_types:
      # This type already exists, so this is an alias. Just pick the first one.
      alias_of = other_types[0]
    if alias_of:
      type_data = alias_of.data
      id_ = alias_of.id
    else:
      id_ = self.id_gen.Get()
    return Type(id_, js_ident, c_ident, c_str, type_data, is_builtin, alias_of)


class Function(object):
  def __init__(self, js_ident, types):
    self.js_ident = js_ident
    self.c_ident = MakeCIdent(js_ident)
    self.types = types


class FunctionsBuilder(object):
  def __init__(self, types_builder):
    self.function_ident_dict = collections.OrderedDict()
    self.types_builder = types_builder

  def AddFunctionDicts(self, fn_dicts, is_builtin=False):
    for fn_dict in fn_dicts:
      self.AddFunctionDict(fn_dict, is_builtin)

  def AddFunctionDict(self, fn_dict, is_builtin=False):
    types = self.types_builder.AddFunctionDict(fn_dict, is_builtin)
    js_ident = fn_dict.name
    function = Function(js_ident, types)
    self.function_ident_dict[js_ident] =  function

  @property
  def functions(self):
    return self.function_ident_dict.values()


def FixTypes(type_dicts, fn_dicts, add_builtin_types=True):
  types_builder = TypesBuilder(add_builtin_types)
  fn_builder = FunctionsBuilder(types_builder)
  types_builder.AddTypeDicts(type_dicts)
  fn_builder.AddFunctionDicts(fn_dicts)

  return types_builder.types, fn_builder.functions
