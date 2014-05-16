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

import helper

def ArgInit(ix, type_):
  if type_.is_pointer:
    # TODO(binji): better way to detect void*?
    if type_.c_ident != 'void_p':
      return 'ARG_VOIDP_CAST(%s, %s);' % (ix, str(type_))
    else:
      return 'ARG_VOIDP(%s);' % ix
  elif type_.is_primitive:
    if type_.is_signed:
      return 'ARG_INT(%s);' % ix
    else:
      return 'ARG_UINT(%s);' % ix
  else:
    raise Exception('Unknown type \"%s\" w/ class %s' % (
        type_, type_.__class__))


def RegisterHandle(type_):
  if type_.is_pointer:
    return 'RegisterHandleVoidp(command->ret_handle, result);'
  elif type_.is_primitive:
    if type_.is_signed:
      return 'RegisterHandleInt32(command->ret_handle, result);'
    else:
      return 'RegisterHandleUint32(command->ret_handle, result);'
  else:
    raise Exception('Unknown type \"%s\" w/ class %s' % (
        type_, type_.__class__))


def FmtArgs(types):
  types = [type_ for type_ in types if type_.GetFormat()]
  return ', '.join(type_.GetFormat() for i, type_ in enumerate(types))


def ArgsCommaSep(args):
  return ', '.join(type_.GetFormatArg(i) for i, type_ in enumerate(args))


def PrintFunction(fn):
  result = 'printf("%s(%s)' % (fn.c_ident, FmtArgs(fn.type.arg_types))
  if not fn.type.return_type.is_void:
    result += ' => %s (%%d)' % fn.type.return_type.GetFormat()
  result += '\\n"'
  if fn.type.arg_types:
    result += ', %s' % ArgsCommaSep(fn.type.arg_types)
  if not fn.type.return_type.is_void:
    result += ', result, command->ret_handle'
  result += ');'
  return result
