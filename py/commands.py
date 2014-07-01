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
    # TODO(binji): better way to detect types?
    if type_.c_ident == 'char_p':
      return 'ARG_CHARP(%s)' % ix
    elif type_.c_ident != 'void_p':
      return 'ARG_VOIDP_CAST(%s, %s);' % (ix, str(type_))
    else:
      return 'ARG_VOIDP(%s);' % ix
  elif type_.is_primitive:
    if type_.is_int:
      if type_.is_long:
        if type_.is_signed:
          return 'ARG_INT_CAST(%s, long);' % ix
        else:
          return 'ARG_UINT_CAST(%s, unsigned long);' % ix
      else:
        if type_.size > 4:
          if type_.is_signed:
            return 'ARG_INT64(%s);' % ix
          else:
            return 'ARG_UINT64(%s);' % ix
        else:
          if type_.is_signed:
            return 'ARG_INT(%s);' % ix
          else:
            return 'ARG_UINT(%s);' % ix
    else:
      if type_.size > 4:
        return 'ARG_FLOAT64(%s);' % ix
      else:
        return 'ARG_FLOAT32(%s);' % ix
  elif type_.is_pepper:
    return 'ARG_VAR(%s);' % ix
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
  elif type_.is_pepper:
    return 'RegisterHandleVar(command->ret_handle, result);'
  else:
    raise Exception('Unknown type \"%s\" w/ class %s' % (
        type_, type_.__class__))


def FmtArgs(types):
  types = [type_ for type_ in types if type_.GetFormat()]
  return ', '.join(type_.GetFormat() for i, type_ in enumerate(types))


def FmtArgsCommaSep(args):
  fmt_args = [type_.GetFormatArg('arg%d' % i) for i, type_ in enumerate(args)]
  return ', '.join(arg for arg in fmt_args if arg is not None)


def ArgsCommaSep(args):
  args = [type_.GetArgString('arg%d' % i) for i, type_ in enumerate(args)]
  return ', '.join(arg for arg in args if arg is not None)


def PrintFunction(c_ident, fn_type):
  result = 'printf("%s(%s)' % (c_ident, FmtArgs(fn_type.arg_types))
  if not fn_type.return_type.is_void:
    result += ' => %s (%%d)' % fn_type.return_type.GetFormat()
  result += '\\n"'
  if fn_type.arg_types:
    result += ', %s' % FmtArgsCommaSep(fn_type.arg_types)
  if not fn_type.return_type.is_void:
    result += ', %s' % fn_type.return_type.GetFormatArg('result')
    result += ', command->ret_handle'
  result += ');'
  return result
