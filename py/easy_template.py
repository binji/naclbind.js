#!/usr/bin/env python
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import copy
import cStringIO
import json
import optparse
import os
import re
import sys


statement_re = re.compile("^\[\[(.*?)\]\]$")  # [[...]]
expr_re = re.compile("\{\{(.*?)\}\}")  # {{...}}
indent_re = re.compile(r'\s*')
statements_begin_re = re.compile(r'^\[\[\[$')
statements_end_re = re.compile(r'^\]\]\]$')

def TemplateToPython(template):
  output = cStringIO.StringIO()
  indent_string = ''
  in_statements = False
  for line in template.splitlines(1):  # 1 => keep line ends
    if statements_begin_re.match(line):
      in_statements = True
      output.write('\n')  # Output a line so error line numbers match up.
      continue
    elif statements_end_re.match(line):
      in_statements = False
      output.write('\n')  # Output a line so error line numbers match up.
      continue
    elif in_statements:
      output.write(line)
      continue

    m = statement_re.match(line)
    if m:
      statement = m.group(1)
      indent_string = indent_re.match(statement).group()
      if statement.rstrip()[-1:] == ':':
        indent_string += '  '
      output.write(statement + '\n')
    else:
      line_ending = ''
      while line and line[-1] in '\\"\n\r':
        line_ending = line[-1] + line_ending
        line = line[:-1]

      ms = list(expr_re.finditer(line))
      if ms:
        # Only replace % with %% outside of the expr matches.
        new_line = ''
        start = 0
        for m in ms:
          new_line += line[start:m.start()].replace('%', '%%')
          new_line += line[m.start():m.end()]
          start = m.end()
        new_line += line[start:].replace('%', '%%')
        line = new_line

        subst_line = r'r"""%s""" %% (%s,)' % (
            re.sub(expr_re, '%s', line),
            ', '.join(re.findall(expr_re, line)))
      else:
        subst_line = r'r"""%s"""' % line

      out_string = r'%s__outfile__.write(%s + %s)' % (
          indent_string,
          subst_line,
          repr(line_ending))
      output.write(out_string + '\n')

  return output.getvalue()


def RunTemplate(srcfile, dstfile, template_dict):
  script = TemplateToPython(srcfile.read())
  template_dict = copy.copy(template_dict)
  template_dict['__outfile__'] = dstfile
  exec script in template_dict


def RunTemplateFile(srcpath, dstpath, template_dict):
  with open(srcpath) as srcfile:
    with open(dstpath, 'w') as dstfile:
      RunTemplate(srcfile, dstfile, template_dict)


def RunTemplateFileIfChanged(srcpath, dstpath, replace):
  dststr = cStringIO.StringIO()
  with open(srcpath) as srcfile:
    RunTemplate(srcfile, dststr, replace)

  if os.path.exists(dstpath):
    with open(dstpath) as dstfile:
      if dstfile.read() == dststr.getvalue():
        return

  with open(dstpath, 'w') as dstfile:
    dstfile.write(dststr.getvalue())


def RunTemplateString(src, template_dict):
  srcstr = cStringIO.StringIO(src)
  dststr = cStringIO.StringIO()
  RunTemplate(srcstr, dststr, template_dict)
  return dststr.getvalue()


def main(args):
  parser = optparse.OptionParser()
  parser.add_option('-j', '--json', help='json file for input')
  options, args = parser.parse_args(args)
  if not args:
    return

  # See http://stackoverflow.com/a/14620633
  class AttrDict(dict):
    def __init__(self, *args, **kwargs):
      super(AttrDict, self).__init__(*args, **kwargs)
      self.__dict__ = self


  with open(args[0]) as f:
    if options.json:
      with open(options.json) as jsonf:
        template_dict = json.load(jsonf, object_hook=AttrDict)
        print RunTemplateString(f.read(), template_dict)
    else:
      print TemplateToPython(f.read())

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
