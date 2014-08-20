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

var type = require('../src/js/type'),
    assert = require('assert');

describe('Type', function() {
  describe('Spell', function() {
    var spell = type.GetSpelling;
    it('should correctly spell primitive types', function() {
      assert.equal(spell(type.void), 'void');
      assert.equal(spell(type.char), 'char');
      assert.equal(spell(type.uchar), 'unsigned char');
      assert.equal(spell(type.schar), 'signed char');
      assert.equal(spell(type.short), 'short');
      assert.equal(spell(type.ushort), 'unsigned short');
      assert.equal(spell(type.int), 'int');
      assert.equal(spell(type.uint), 'unsigned int');
      assert.equal(spell(type.long), 'long');
      assert.equal(spell(type.ulong), 'unsigned long');
      assert.equal(spell(type.longlong), 'long long');
      assert.equal(spell(type.ulonglong), 'unsigned long long');
      assert.equal(spell(type.float), 'float');
      assert.equal(spell(type.double), 'double');
      assert.equal(spell(type.wchar), 'wchar_t');
    });

    it('should correctly spell qualified primitive types', function() {
      var cv = type.CONST | type.VOLATILE,
          cvr = type.CONST | type.VOLATILE | type.RESTRICT;
      assert.equal(spell(type.Void(type.CONST)), 'const void');
      assert.equal(spell(type.Numeric(type.CHAR, type.CONST)), 'const char');
      assert.equal(spell(type.Numeric(type.INT, type.CONST)), 'const int');
      assert.equal(spell(type.Numeric(type.INT, cv)), 'const volatile int');
      assert.equal(spell(type.Numeric(type.FLOAT, cvr)),
                   'const volatile restrict float');
    });

    it('should correctly spell record types', function() {
      var s = type.Record('s', type.Field('field', type.int, 0)),
          u = type.Record('u', type.Field('field', type.int, 0), type.UNION);
      assert.equal(spell(s), 'struct s');
      assert.equal(spell(u), 'union u');
    });

    it('should correctly spell enum types', function() {
      assert.equal(spell(type.Enum('name')), 'enum name');
    });

    it('should correctly spell typedef types', function() {
      var t = type.Typedef('foo', type.int);
      assert.equal(spell(t), 'foo');
    });

    it('should correctly spell pointer types', function() {
      var s = type.Record('myStruct', type.Field('field', type.int, 0)),
          e = type.Enum('myEnum'),
          t = type.Typedef('myTypedef', type.int),
          f = type.Function(type.void, [type.int]);
      assert.equal(spell(type.Pointer(type.void)), 'void *');
      assert.equal(spell(type.Pointer(type.int)), 'int *');
      assert.equal(spell(type.Pointer(s)), 'struct myStruct *');
      assert.equal(spell(type.Pointer(e)), 'enum myEnum *');
      assert.equal(spell(type.Pointer(t)), 'myTypedef *');
      assert.equal(spell(type.Pointer(f)), 'void (*)(int)');
      assert.equal(spell(type.Pointer(type.Pointer(type.int))), 'int **');
    });

    it('should correctly spell qualified pointer types', function() {
      var Kc = type.Numeric(type.CHAR, type.CONST),
          PKc = type.Pointer(Kc),
          PKPc = type.Pointer(type.Pointer(type.char, type.CONST)),
          PKt = type.Pointer(type.Typedef('foo', type.char, type.CONST)),
          PKPKc = type.Pointer(type.Pointer(Kc, type.CONST)),
          PKVi = type.Pointer(type.Numeric(type.INT,
                                           type.CONST | type.VOLATILE));
      assert.equal(spell(PKc), 'const char *');
      assert.equal(spell(PKPc), 'char *const *');
      assert.equal(spell(PKt), 'const foo *');
      assert.equal(spell(PKPKc), 'const char *const *');
      assert.equal(spell(PKVi), 'const volatile int *');
    });

    it('should correctly spell function types', function() {
      var PKc = type.Pointer(type.Numeric(type.CHAR, type.CONST)),
          f1 = type.Function(type.int, [type.int, type.int]),
          f2 = type.Function(type.int, [PKc], 0, type.VARIADIC),
          f3 = type.Pointer(type.Function(type.void, [type.int])),
          f4 = type.Function(f3, [type.int, f3]),
          f5 = type.Function(type.void, []);
      assert.equal(spell(f1), 'int (int, int)');
      assert.equal(spell(f2), 'int (const char *, ...)');
      assert.equal(spell(f4), 'void (*(int, void (*)(int)))(int)');
      assert.equal(spell(f4, 'signal'),
                   'void (*signal(int, void (*)(int)))(int)');
      assert.equal(spell(f5), 'void (void)');

      // Illegal variadic.
      // var FivzE = type.Function(type.int, [], type.VARIADIC);
      // assert.equal(spell(FivzE), 'int (...)');
    });

    it('should handle spelling precedence', function() {
      var IA = type.IncompleteArray,
          P = type.Pointer,
          F = type.Function,
          i = type.int,
          A_PFiiE = IA(P(F(i, [i]))),
          PA_PFiiE = P(IA(P(F(i, [i])))),
          PFPFPivEiE = P(F(P(F(P(i), [])), [i]));
      assert.equal(spell(A_PFiiE), 'int (*[])(int)');
      assert.equal(spell(PA_PFiiE), 'int (*(*)[])(int)');
      assert.equal(spell(PA_PFiiE, 'foo'), 'int (*(*foo)[])(int)');
      assert.equal(spell(PFPFPivEiE), 'int *(*(*)(int))(void)');

      // Illegal return type.
      // var PFA_iiE = type.Pointer(type.Function(
      //       type.IncompleteArray(type.int), [type.int]));
      //assert.equal(spell(PFA_iiE), 'int (*)(int)[]');
    });
  });
});
