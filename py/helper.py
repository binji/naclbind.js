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


FIRST_ID = 80

BUILTIN_TYPES = [
  {'name': 'void', 'kind': 'void'},
  {'name': 'int8', 'kind': 'prim', 'size': 1, 'signed': True, 'int': True},
  {'name': 'uint8', 'kind': 'prim', 'size': 1, 'signed': False, 'int': True},
  {'name': 'int16', 'kind': 'prim', 'size': 2, 'signed': True, 'int': True},
  {'name': 'uint16', 'kind': 'prim', 'size': 2, 'signed': False, 'int': True},
  {'name': 'int32', 'kind': 'prim', 'size': 4, 'signed': True, 'int': True},
  {'name': 'uint32', 'kind': 'prim', 'size': 4, 'signed': False, 'int': True},
  {'name': 'int64', 'kind': 'prim', 'size': 8, 'signed': True, 'int': True},
  {'name': 'uint64', 'kind': 'prim', 'size': 8, 'signed': False, 'int': True},
  {'name': 'float32', 'kind': 'prim', 'size': 4, 'int': False},
  {'name': 'float64', 'kind': 'prim', 'size': 8, 'int': False},
  {'name': 'void$', 'kind': 'pointer', 'base': 'void'},
  {'name': 'int8$', 'kind': 'pointer', 'base': 'int8'},
  {'name': 'uint8$', 'kind': 'pointer', 'base': 'uint8'},
  {'name': 'int16$', 'kind': 'pointer', 'base': 'int16'},
  {'name': 'uint16$', 'kind': 'pointer', 'base': 'uint16'},
  {'name': 'int32$', 'kind': 'pointer', 'base': 'int32'},
  {'name': 'uint32$', 'kind': 'pointer', 'base': 'uint32'},
  {'name': 'int64$', 'kind': 'pointer', 'base': 'int64'},
  {'name': 'uint64$', 'kind': 'pointer', 'base': 'uint64'},
  {'name': 'float32$', 'kind': 'pointer', 'base': 'float32'},
  {'name': 'float64$', 'kind': 'pointer', 'base': 'float64'},
  {'name': 'void$$', 'kind': 'pointer', 'base': 'void$'},
  {'name': 'Var', 'kind': 'pepper', 'prototype': 'undefined'},
  {'name': 'ArrayBuffer', 'kind': 'pepper', 'prototype': 'ArrayBuffer'},
  {'name': 'Array', 'kind': 'pepper', 'prototype': 'Array'},
  {'name': 'Dictionary', 'kind': 'pepper', 'prototype': 'Object'},
  {'name': 'String', 'kind': 'pepper', 'prototype': 'String'},
]
# Convert each element to AttrDict
BUILTIN_TYPES = [AttrDict(d) for d in BUILTIN_TYPES]

PRIM_NAMES = {
  'int8': 'int8_t',
  'uint8': 'uint8_t',
  'int16': 'int16_t',
  'uint16': 'uint16_t',
  'int32': 'int32_t',
  'uint32': 'uint32_t',
  'int64': 'int64_t',
  'uint64': 'uint64_t',
  'float32': 'float',
  'float64': 'double',
}


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


def MakeCName(name):
  return name.replace('$', '_p')


class Type(object):
  def __init__(self, id_):
    self.id = id_

  def GetFormat(self):
    return None

  def GetFormatArg(self, ix):
    return None

  def _GetDefaultFormatArg(self, ix):
    return 'arg%d' % ix

  def IsVoid(self):
    return False

  def IsPointer(self):
    return False

  def IsPrimitive(self):
    return False


class VoidType(Type):
  def __init__(self, id_, typ, _):
    Type.__init__(self, id_)
    self.name = typ.name
    self.cname = typ.name

  def __str__(self):
    return self.name

  def IsVoid(self):
    return True


class PrimType(Type):
  def __init__(self, id_, typ, _):
    Type.__init__(self, id_)
    self.name = typ.name
    self.cname = typ.name
    self.str = PRIM_NAMES[typ.name]
    self.signed = ('signed' in typ) and typ.signed
    self.size = typ.size
    self.is_int = typ.int

  def GetFormat(self):
    if self.size <= 4:
      return '%d' if self.signed else '%u'
    else:
      return '%lld' if self.signed else '%llu'

  def GetFormatArg(self, ix):
    return self._GetDefaultFormatArg(ix)

  def __str__(self):
    return self.str

  def IsPrimitive(self):
    return True


class StructType(Type):
  def __init__(self, id_, typ, _):
    Type.__init__(self, id_)
    self.name = typ.name
    self.cname = typ.name

  def __str__(self):
    return self.name


class PepperType(Type):
  def __init__(self, id_, typ, _):
    Type.__init__(self, id_)
    self.name = typ.name
    self.cname = typ.name

  def GetFormat(self):
    # TODO(binji): better output here.
    return '<Var>'

  def GetFormatArg(self, ix):
    return None

  def __str__(self):
    return self.name


class PointerType(Type):
  def __init__(self, id_, typ, types):
    Type.__init__(self, id_)
    self.name = typ.name
    self.cname = MakeCName(typ.name)
    self.str = str(types.all_types[typ.base]) + '*'

  def GetFormat(self):
    return '%p'

  def GetFormatArg(self, ix):
    return self._GetDefaultFormatArg(ix)

  def __str__(self):
    return self.str

  def IsPointer(self):
    return True


class AliasType(Type):
  def __init__(self, _, typ, types):
    self.alias = types.all_types[typ.alias]
    assert not isinstance(self.alias, AliasType)

    Type.__init__(self, self.alias.id)
    self.name = typ.name
    self.cname = MakeCName(typ.name)
    self.str = typ.str

  def __str__(self):
    return self.str

  def __getattr__(self, name):
    return getattr(self.alias, name)

  def GetFormat(self):
    return self.alias.GetFormat()

  def GetFormatArg(self, ix):
    return self.alias.GetFormatArg(ix)

  def IsPointer(self):
    return self.alias.IsPointer()

  def IsPrimitive(self):
    return self.alias.IsPrimitive()


class FunctionType(Type):
  def __init__(self, id_, fn, types):
    Type.__init__(self, id_)
    self.name = fn.name
    self.arg_types = [types.all_types[arg] for arg in fn.args]
    self.return_type = types.all_types[fn.result]

    args = ', '.join(map(str, self.arg_types))
    self.str = '%s (*)(%s)' % (self.return_type, args)

  def __str__(self):
    return self.str


KIND_TO_CONSTRUCTOR = {
  'void': VoidType,
  'prim': PrimType,
  'struct': StructType,
  'pepper': PepperType,
  'pointer': PointerType,
}


def FixTypes(type_dicts, alias_dicts, fn_dicts):
  types = Types()
  types.AddTypes(type_dicts)
  types.AddAliases(alias_dicts)
  types.AddFunctionTypes(fn_dicts)
  return types


#def FixFunctions(types, fn_dicts):
#  types.AddFunctionTypes(fn_dicts)
#  functions = []
#  for fn_dict in fn_dicts:
#    functions.append(Function(types, fn_dict))
#  return functions


class Types(object):
  def __init__(self):
    self.next_id = 1
    self.all_types = collections.OrderedDict()
    self.no_builtins = collections.OrderedDict()
    self.function_types = collections.OrderedDict()
    self.AddTypes(BUILTIN_TYPES, builtins=True)
    # TODO(binji): hack because not all types are specified in BUILTIN_TYPES
    self.next_id = FIRST_ID

  def AddTypes(self, type_dicts, builtins=False):
    for type_dict in type_dicts:
      typ = self.MakeType(self.next_id, type_dict)
      self.all_types[typ.name] = typ
      if not builtins:
        self.no_builtins[typ.name] = typ
      self.next_id += 1

  def MakeType(self, id_, type_dict):
    return KIND_TO_CONSTRUCTOR[type_dict.kind](id_, type_dict, self)

  def AddAliases(self, alias_dicts):
    for alias_dict in alias_dicts:
      # Aliases don't have a unique id.
      typ = AliasType(0, alias_dict, self)
      self.all_types[typ.name] = typ

  def AddFunctionTypes(self, fn_dicts):
    for fn_dict in fn_dicts:
      typ = FunctionType(self.next_id, fn_dict, self)
      self.all_types[typ.name] = typ
      self.function_types[typ.name] = typ
      self.next_id += 1
