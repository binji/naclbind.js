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
    assert = require('assert'),
    spell = type.getSpelling,
    qual = type.describeQualifier,
    canon = type.getCanonical,
    viable = type.getBestViableFunction,
    C = type.CONST,
    CV = type.CONST | type.VOLATILE,
    CR = type.CONST | type.RESTRICT,
    CVR = type.CONST | type.VOLATILE | type.RESTRICT,
    V = type.VOLATILE,
    VR = type.VOLATILE | type.RESTRICT,
    R = type.RESTRICT;

describe('Type', function() {
  describe('is{Less,More}{,OrEqually}Qualified', function() {
    it('should work properly for all qualifiers', function() {
      var OK = [
            [0, C], [0, V], [0, R], [0, CV], [0, CR], [0, VR], [0, CVR],
            [C, CV], [C, CR], [C, CVR],
            [V, CV], [V, VR], [V, CVR],
            [R, CR], [R, VR], [R, CVR],
            [CV, CVR],
            [CR, CVR],
            [VR, CVR]
          ],
          Q = [0, C, V, R, CV, CR, VR, CVR],
          NotOK = [];

      Q.forEach(function(q1) {
        Q.forEach(function(q2) {
          var found = false;
          OK.forEach(function(x) {
            if (x[0] === q1 && x[1] === q2) {
              found = true;
            }
          });

          if (!found) {
            NotOK.push([q1, q2]);
          }
        });
      });

      OK.forEach(function(a) {
        var q1 = a[0], q2 = a[1];
        assert(type.isLessQualified(q1, q2),
               'Expected ' + qual(q1) + ' < ' + qual(q2));
        assert(type.isLessOrEquallyQualified(q1, q2),
               'Expected ' + qual(q1) + ' <= ' + qual(q2));
        assert(type.isMoreQualified(q2, q1),
               'Expected ' + qual(q2) + ' > ' + qual(q1));
        assert(type.isMoreOrEquallyQualified(q2, q1),
               'Expected ' + qual(q2) + ' >= ' + qual(q1));
      });

      NotOK.forEach(function(a) {
        var q1 = a[0], q2 = a[1];
        assert(!type.isLessQualified(q1, q2),
               'Expected ' + qual(q1) + ' !< ' + qual(q2));
        assert(!type.isMoreQualified(q2, q1),
               'Expected ' + qual(q2) + ' !> ' + qual(q1));
        if (q1 === q2) {
          assert(type.isLessOrEquallyQualified(q1, q2),
                 'Expected ' + qual(q1) + ' <= ' + qual(q2));
          assert(type.isMoreOrEquallyQualified(q2, q1),
                 'Expected ' + qual(q2) + ' >= ' + qual(q1));
        }
      });
    });
  });

  describe('Create', function() {
    describe('Type', function() {
      it('should have valid cv', function() {
        [null, -1, 10, 'const'].forEach(function(badCv) {
          assert.throws(function() { type.Void(badCv); });
          assert.throws(function() { type.Numeric(type.INT, badCv); });
          assert.throws(function() { type.Pointer(type.int, badCv); });
          assert.throws(function() {
            type.Record('s', [type.Field('f', type.int, 0)], type.STRUCT, badCv);
          });
          assert.throws(function() { type.Enum('e', badCv); });
          assert.throws(function() { type.Typedef('t', type.int, badCv); });
        });
      });
    });

    describe('Numeric', function() {
      it('should have valid kind', function() {
        [null, -1, 26, 'int'].forEach(function(badKind) {
          assert.throws(function() { type.Numeric(badKind); });
        });
      });
    });

    describe('Pointer', function() {
      it('should throw creating pointer with bad pointee', function() {
        [null, 'int', 100].forEach(function(badPointee) {
          assert.throws(function() { type.Pointer(badPointee); }, /pointee/);
        });
      });
    });

    describe('Enum', function() {
      it('should throw creating enum with bad tag', function() {
        [undefined, 100].forEach(function(badTag) {
          assert.throws(function() { type.Enum(badTag); });
        });
      });
    });

    describe('Function', function() {
      it('should throw creating functions with bad resultType', function() {
        [undefined, null, 100, 'int'].forEach(function(badType) {
          assert.throws(function() {
            type.Function(badType, [type.int]);
          }, /resultType/);
        });
      });

      it('should throw creating functions with bad argTypes', function() {
        [undefined, null, 100, 'int'].forEach(function(badType) {
          assert.throws(function() {
            type.Function(type.void, [badType]);
          }, /argTypes/);
        });
      });

      it('should throw creating functions that return arrays', function() {
        assert.throws(function() {
          type.Function(type.Array(type.int, 2), []);
        });
      });

      it('should throw creating functions with non-array argTypes', function() {
        assert.throws(function() {
          type.Function(type.void, type.int);
        });
      });

      it('should throw creating variadic functions with 0 args', function() {
        assert.throws(function() {
          type.Function(type.void, [], type.VARIADIC);
        });
      });

      it('should throw creating function with void argTypes', function() {
        assert.throws(function() {
          type.Function(type.void, [type.void]);
        });
      });
    });

    describe('Array', function() {
      it('should throw creating an array with bad elementType', function() {
        [undefined, null, 100, 'int'].forEach(function(badType) {
          assert.throws(function() {
            type.Array(badType, 2);
          }, /elementType/);

          assert.throws(function() {
            type.IncompleteArray(badType);
          }, /elementType/);
        });
      });

      it('should throw creating an array with bad arraySize', function() {
        [undefined, null, '100'].forEach(function(badArraySize) {
          assert.throws(function() {
            type.Array(type.int, badArraySize);
          }, /arraySize/);
        });
      });

      it('should throw creating an array of voids', function() {
        assert.throws(function() {
          type.Array(type.void, 2);
        });

        assert.throws(function() {
          type.IncompleteArray(type.void);
        });
      });
    });

    describe('Record', function() {
      it('should throw creating a record with bad name', function() {
        // Name should be string or null (for anonymous).
        [undefined, 100].forEach(function(badTag) {
          assert.throws(function() {
            type.Record(badTag, [type.Field('f', type.int, 0)]);
          });
        });
      });

      it('should throw creating a record with bad fields', function() {
        // No array
        assert.throws(function() {
          type.Record('foo', type.Field('f', type.int, 0));
        });

        // Raw object instead of type.Field
        assert.throws(function() {
          type.Record('foo', [{name: 'f', type: type.int, offset: 0}]);
        });

        assert.throws(function() {
          type.Record('foo', [undefined]);
        });

        // Field name must be string or null (for anonymous)
        assert.throws(function() {
          type.Record('foo', [type.Field(1234, type.int, 0)]);
        });

        // Field must have valid type
        assert.throws(function() {
          type.Record('foo', [type.Field('f', null, 0)]);
        });

        // Field offset must be number
        assert.throws(function() {
          type.Record('foo', [type.Field('f', type.int, 'none')]);
        });
      });

      it('should throw creating a record with bad struct/union', function() {
        // Should be type.STRUCT/type.UNION (bool)
        assert.throws(function() {
          type.Record('foo', [type.Field('f', type.int, 0)], 'struct');
        });

        assert.throws(function() {
          type.Record('foo', [type.Field('f', type.int, 0)], null);
        });
      });
    });
  });

  describe('Spell', function() {
    it('should correctly spell primitive types', function() {
      assert.strictEqual(spell(type.void), 'void');
      assert.strictEqual(spell(type.char), 'char');
      assert.strictEqual(spell(type.uchar), 'unsigned char');
      assert.strictEqual(spell(type.schar), 'signed char');
      assert.strictEqual(spell(type.short), 'short');
      assert.strictEqual(spell(type.ushort), 'unsigned short');
      assert.strictEqual(spell(type.int), 'int');
      assert.strictEqual(spell(type.uint), 'unsigned int');
      assert.strictEqual(spell(type.long), 'long');
      assert.strictEqual(spell(type.ulong), 'unsigned long');
      assert.strictEqual(spell(type.longlong), 'long long');
      assert.strictEqual(spell(type.ulonglong), 'unsigned long long');
      assert.strictEqual(spell(type.float), 'float');
      assert.strictEqual(spell(type.double), 'double');
      assert.strictEqual(spell(type.wchar), 'wchar_t');
    });

    it('should correctly spell qualified primitive types', function() {
      var cv = type.CONST | type.VOLATILE,
          cvr = type.CONST | type.VOLATILE | type.RESTRICT;
      assert.strictEqual(spell(type.Void(type.CONST)), 'const void');
      assert.strictEqual(spell(type.char.qualify(type.CONST)), 'const char');
      assert.strictEqual(spell(type.int.qualify(type.CONST)), 'const int');
      assert.strictEqual(spell(type.int.qualify(cv)), 'const volatile int');
      assert.strictEqual(spell(type.float.qualify(cvr)),
                   'const volatile restrict float');
    });

    it('should correctly spell record types', function() {
      var s = type.Record('s', [type.Field('field', type.int, 0)]),
          u = type.Record('u', [type.Field('field', type.int, 0)], type.UNION);
      assert.strictEqual(spell(s), 'struct s');
      assert.strictEqual(spell(u), 'union u');
    });

    it('should correctly spell enum types', function() {
      assert.strictEqual(spell(type.Enum('name')), 'enum name');
    });

    it('should correctly spell typedef types', function() {
      var t = type.Typedef('foo', type.int);
      assert.strictEqual(spell(t), 'foo');
    });

    it('should correctly spell pointer types', function() {
      var s = type.Record('myStruct', [type.Field('field', type.int, 0)]),
          e = type.Enum('myEnum'),
          t = type.Typedef('myTypedef', type.int),
          f = type.Function(type.void, [type.int]);
      assert.strictEqual(spell(type.Pointer(type.void)), 'void *');
      assert.strictEqual(spell(type.Pointer(type.int)), 'int *');
      assert.strictEqual(spell(type.Pointer(s)), 'struct myStruct *');
      assert.strictEqual(spell(type.Pointer(e)), 'enum myEnum *');
      assert.strictEqual(spell(type.Pointer(t)), 'myTypedef *');
      assert.strictEqual(spell(type.Pointer(f)), 'void (*)(int)');
      assert.strictEqual(spell(type.Pointer(type.Pointer(type.int))), 'int **');
    });

    it('should correctly spell qualified pointer types', function() {
      var Kc = type.char.qualify(type.CONST),
          PKc = type.Pointer(Kc),
          PKPc = type.Pointer(type.Pointer(type.char, type.CONST)),
          PKt = type.Pointer(type.Typedef('foo', type.char, type.CONST)),
          PKPKc = type.Pointer(type.Pointer(Kc, type.CONST)),
          PKVi = type.Pointer(type.int.qualify(type.CONST | type.VOLATILE));
      assert.strictEqual(spell(PKc), 'const char *');
      assert.strictEqual(spell(PKPc), 'char *const *');
      assert.strictEqual(spell(PKt), 'const foo *');
      assert.strictEqual(spell(PKPKc), 'const char *const *');
      assert.strictEqual(spell(PKVi), 'const volatile int *');
    });

    it('should correctly spell function types', function() {
      var PKc = type.Pointer(type.char.qualify(type.CONST)),
          f1 = type.Function(type.int, [type.int, type.int]),
          f2 = type.Function(type.int, [PKc], type.VARIADIC),
          f3 = type.Pointer(type.Function(type.void, [type.int])),
          f4 = type.Function(f3, [type.int, f3]),
          f5 = type.Function(type.void, []);
      assert.strictEqual(spell(f1), 'int (int, int)');
      assert.strictEqual(spell(f2), 'int (const char *, ...)');
      assert.strictEqual(spell(f4), 'void (*(int, void (*)(int)))(int)');
      assert.strictEqual(spell(f4, 'signal'),
                   'void (*signal(int, void (*)(int)))(int)');
      assert.strictEqual(spell(f5), 'void (void)');
    });

    it('should handle spelling precedence', function() {
      var IA = type.IncompleteArray,
          P = type.Pointer,
          F = type.Function,
          i = type.int,
          A_PFiiE = IA(P(F(i, [i]))),
          PA_PFiiE = P(IA(P(F(i, [i])))),
          PFPFPivEiE = P(F(P(F(P(i), [])), [i]));
      assert.strictEqual(spell(A_PFiiE), 'int (*[])(int)');
      assert.strictEqual(spell(PA_PFiiE), 'int (*(*)[])(int)');
      assert.strictEqual(spell(PA_PFiiE, 'foo'), 'int (*(*foo)[])(int)');
      assert.strictEqual(spell(PFPFPivEiE), 'int *(*(*)(int))(void)');
    });
  });

  describe('Canonical', function() {
    it('should ignore types with typedefs', function() {
      var v = type.void,
          i = type.int,
          Pc = type.Pointer(type.char),
          A2_c = type.Array(type.char, 2),
          A_c = type.IncompleteArray(type.char),
          s = type.Record('s', [type.Field('f', type.int, 0)]),
          e = type.Enum('e'),
          FiiE = type.Function(type.int, [type.int]);
      assert.strictEqual(canon(v), v);
      assert.strictEqual(canon(i), i);
      assert.strictEqual(canon(Pc), Pc);
      assert.strictEqual(canon(A2_c), A2_c);
      assert.strictEqual(canon(A_c), A_c);
      assert.strictEqual(canon(s), s);
      assert.strictEqual(canon(e), e);
      assert.strictEqual(canon(FiiE), FiiE);
    });

    it('should reduce typedefs of all types', function() {
      var types = [
        type.void,
        type.int,
        type.Pointer(type.char),
        type.Array(type.char, 2),
        type.IncompleteArray(type.char),
        type.Record('s', [type.Field('f', type.int, 0)]),
        type.Enum('e'),
        type.Function(type.int, [type.int])
      ];

      types.forEach(function(t) {
        var typedef = type.Typedef('t', t);
        assert(canon(typedef).equals(t));
      });
    });

    it('should reduce typedefs of child types', function() {
      var t = type.Typedef('t', type.char),
          tt = type.Typedef('tt', t);

      [t, tt].forEach(function(x) {
        var Px = type.Pointer(x),
            A2_x = type.Array(x, 2),
            A_x = type.IncompleteArray(x),
            FvxE = type.Function(type.void, [x]),
            FxvE = type.Function(x, []);

        assert(canon(x).equals(type.char));
        assert(canon(Px).equals(type.Pointer(type.char)));
        assert(canon(A2_x).equals(type.Array(type.char, 2)));
        assert(canon(A_x).equals(type.IncompleteArray(type.char)));
        assert(canon(FvxE).equals(type.Function(type.void, [type.char])));
        assert(canon(FxvE).equals(type.Function(type.char, [])));
      });
    });

    it('should combine typedef qualifiers', function() {
      var Kc = type.char.qualify(type.CONST),
          Kt = type.Typedef('Kt', Kc, type.CONST),
          Vt = type.Typedef('Vt', Kc, type.VOLATILE);

      assert(canon(Kt).equals(Kc));  // Extra const is ignored.
      assert(canon(Vt).equals(type.char.qualify(type.CONST | type.VOLATILE)));
    });
  });

  describe('Cast', function() {
    function spellTypedef(t) {
      if (t.kind === type.TYPEDEF) {
        return 'typedef of ' + spellTypedef(t.alias);
      }
      return spell(t);
    }

    function assertCastHelper(from, to, expected) {
      var actual = from.canCastTo(to);
      var msg = 'Cast from "' + spellTypedef(from) + '" -> "' +
                                spellTypedef(to) + '": ' +
                'expected: ' + expected + ' actual: ' + actual;
      assert.strictEqual(actual, expected, msg);
    }

    function assertCast(from, to, expected) {
      var tfrom = type.Typedef('from', from),
          ttfrom = type.Typedef('fromfrom', tfrom),
          tto = type.Typedef('to', to),
          ttto = type.Typedef('toto', tto);

      // console.log(spell(from), '=>', spell(to), expected);

      assertCastHelper(from, to, expected);
      assertCastHelper(from, tto, expected);
      assertCastHelper(tfrom, to, expected);
      assertCastHelper(tfrom, tto, expected);
      assertCastHelper(ttfrom, to, expected);
      assertCastHelper(from, ttto, expected);
      assertCastHelper(ttfrom, ttto, expected);
    }

    describe('Void', function() {
      it('should allow cast of void -> void', function() {
        assertCast(type.void, type.void, type.CAST_OK_EXACT);
      });

      it('should fail cast of void -> anything else', function() {
        var e = type.Enum('e'),
            s = type.Record('s', [type.Field('f', type.int, 0)]),
            u = type.Record('s', [type.Field('f', type.int, 0)], type.UNION),
            f = type.Function(type.void, [type.int]),
            fp = type.Pointer(f),
            p = type.Pointer(type.void),
            a = type.Array(type.char, 2),
            ia = type.IncompleteArray(type.char);
        assertCast(type.void, e, type.CAST_ERROR);
        assertCast(type.void, s, type.CAST_ERROR);
        assertCast(type.void, u, type.CAST_ERROR);
        assertCast(type.void, f, type.CAST_ERROR);
        assertCast(type.void, fp, type.CAST_ERROR);
        assertCast(type.void, p, type.CAST_ERROR);
        assertCast(type.void, a, type.CAST_ERROR);
        assertCast(type.void, ia, type.CAST_ERROR);
        assertCast(type.void, type.bool, type.CAST_ERROR);
        assertCast(type.void, type.char, type.CAST_ERROR);
        assertCast(type.void, type.short, type.CAST_ERROR);
        assertCast(type.void, type.int, type.CAST_ERROR);
        assertCast(type.void, type.long, type.CAST_ERROR);
        assertCast(type.void, type.longlong, type.CAST_ERROR);
        assertCast(type.void, type.uchar, type.CAST_ERROR);
        assertCast(type.void, type.ushort, type.CAST_ERROR);
        assertCast(type.void, type.uint, type.CAST_ERROR);
        assertCast(type.void, type.ulong, type.CAST_ERROR);
        assertCast(type.void, type.ulonglong, type.CAST_ERROR);
        assertCast(type.void, type.float, type.CAST_ERROR);
        assertCast(type.void, type.double, type.CAST_ERROR);
      });
    });

    describe('Numeric', function() {
      it('should allow cast of numeric -> larger numeric', function() {
        assertCast(type.bool, type.char, type.CAST_OK_PROMOTION);
        assertCast(type.char, type.short, type.CAST_OK_PROMOTION);
        assertCast(type.short, type.int, type.CAST_OK_PROMOTION);
        assertCast(type.int, type.long, type.CAST_OK_PROMOTION);
        assertCast(type.long, type.longlong, type.CAST_OK_PROMOTION);
        assertCast(type.uchar, type.ushort, type.CAST_OK_PROMOTION);
        assertCast(type.ushort, type.uint, type.CAST_OK_PROMOTION);
        assertCast(type.uint, type.ulong, type.CAST_OK_PROMOTION);
        assertCast(type.ulong, type.ulonglong, type.CAST_OK_PROMOTION);
        assertCast(type.float, type.double, type.CAST_OK_PROMOTION);
      });

      it('should allow cast of unsigned -> larger signed', function() {
        assertCast(type.uchar, type.short, type.CAST_OK_PROMOTION);
        assertCast(type.ushort, type.int, type.CAST_OK_PROMOTION);
        assertCast(type.uint, type.long, type.CAST_OK_PROMOTION);
        assertCast(type.ulong, type.longlong, type.CAST_OK_PROMOTION);
      });

      it('should warn on cast of unsigned <-> equal-sized signed', function() {
        assertCast(type.uchar, type.char, type.CAST_SIGNED_UNSIGNED);
        assertCast(type.ushort, type.short, type.CAST_SIGNED_UNSIGNED);
        assertCast(type.uint, type.int, type.CAST_SIGNED_UNSIGNED);
        assertCast(type.ulong, type.long, type.CAST_SIGNED_UNSIGNED);
        assertCast(type.ulonglong, type.longlong, type.CAST_SIGNED_UNSIGNED);

        assertCast(type.char, type.uchar, type.CAST_SIGNED_UNSIGNED);
        assertCast(type.short, type.ushort, type.CAST_SIGNED_UNSIGNED);
        assertCast(type.int, type.uint, type.CAST_SIGNED_UNSIGNED);
        assertCast(type.long, type.ulong, type.CAST_SIGNED_UNSIGNED);
        assertCast(type.longlong, type.ulonglong, type.CAST_SIGNED_UNSIGNED);
      });

      it('should warn on cast of numeric -> smaller numeric', function() {
        assertCast(type.char, type.bool, type.CAST_TRUNCATE);
        assertCast(type.short, type.char, type.CAST_TRUNCATE);
        assertCast(type.int, type.short, type.CAST_TRUNCATE);
        assertCast(type.long, type.int, type.CAST_TRUNCATE);
        assertCast(type.longlong, type.long, type.CAST_TRUNCATE);
        assertCast(type.ushort, type.uchar, type.CAST_TRUNCATE);
        assertCast(type.uint, type.ushort, type.CAST_TRUNCATE);
        assertCast(type.ulong, type.uint, type.CAST_TRUNCATE);
        assertCast(type.ulonglong, type.ulong, type.CAST_TRUNCATE);
        assertCast(type.double, type.float, type.CAST_TRUNCATE);
      });

      it('should warn on cast of integral -> pointer-like', function() {
        var c = type.char,
            v = type.void,
            p = type.Pointer(v),
            a = type.Array(c, 2),
            ia = type.IncompleteArray(c);
        [p, a, ia].forEach(function(x) {
          assertCast(type.char, x, type.CAST_INT_TO_POINTER);
          assertCast(type.short, x, type.CAST_INT_TO_POINTER);
          assertCast(type.int, x, type.CAST_INT_TO_POINTER);
          assertCast(type.long, x, type.CAST_INT_TO_POINTER);
          assertCast(type.longlong, x, type.CAST_INT_TO_POINTER);
          assertCast(type.uchar, x, type.CAST_INT_TO_POINTER);
          assertCast(type.ushort, x, type.CAST_INT_TO_POINTER);
          assertCast(type.uint, x, type.CAST_INT_TO_POINTER);
          assertCast(type.ulong, x, type.CAST_INT_TO_POINTER);
          assertCast(type.ulonglong, x, type.CAST_INT_TO_POINTER);
        });
      });

      it('should fail to cast float -> pointer-like', function() {
        var c = type.char,
            v = type.void,
            p = type.Pointer(v),
            a = type.Array(c, 2),
            ia = type.IncompleteArray(c);
        [p, a, ia].forEach(function(x) {
          assertCast(type.float, x, type.CAST_ERROR);
          assertCast(type.double, x, type.CAST_ERROR);
        });
      });

      it('should warn on cast of integral -> enum', function() {
        var e = type.Enum('e');
        assertCast(type.char, e, type.CAST_INT_TO_ENUM);
        assertCast(type.short, e, type.CAST_INT_TO_ENUM);
        assertCast(type.int, e, type.CAST_INT_TO_ENUM);
        assertCast(type.long, e, type.CAST_INT_TO_ENUM);
        assertCast(type.longlong, e, type.CAST_INT_TO_ENUM);
        assertCast(type.uchar, e, type.CAST_INT_TO_ENUM);
        assertCast(type.ushort, e, type.CAST_INT_TO_ENUM);
        assertCast(type.uint, e, type.CAST_INT_TO_ENUM);
        assertCast(type.ulong, e, type.CAST_INT_TO_ENUM);
        assertCast(type.ulonglong, e, type.CAST_INT_TO_ENUM);
      });

      it('should fail to cast float -> enum', function() {
        var e = type.Enum('e');
        assertCast(type.float, e, type.CAST_ERROR);
        assertCast(type.double, e, type.CAST_ERROR);
      });

      it('should fail to cast numeric -> record, void, function', function() {
        var s = type.Record('s', [type.Field('f', type.int, 0)]),
            u = type.Record('s', [type.Field('f', type.int, 0)], type.UNION),
            v = type.void,
            f = type.Function(type.void, [type.int]);
        [s, u, v, f].forEach(function(x) {
          assertCast(type.char, x, type.CAST_ERROR);
          assertCast(type.short, x, type.CAST_ERROR);
          assertCast(type.int, x, type.CAST_ERROR);
          assertCast(type.long, x, type.CAST_ERROR);
          assertCast(type.longlong, x, type.CAST_ERROR);
          assertCast(type.uchar, x, type.CAST_ERROR);
          assertCast(type.ushort, x, type.CAST_ERROR);
          assertCast(type.uint, x, type.CAST_ERROR);
          assertCast(type.ulong, x, type.CAST_ERROR);
          assertCast(type.ulonglong, x, type.CAST_ERROR);
          assertCast(type.float, x, type.CAST_ERROR);
          assertCast(type.double, x, type.CAST_ERROR);
        });
      });
    });

    describe('Pointer', function() {
      var v = type.void,
          c = type.char,
          e = type.Enum('e'),
          s = type.Record('s', [type.Field('f', type.int, 0)]),
          u = type.Record('s', [type.Field('f', type.int, 0)], type.UNION),
          f = type.Function(type.void, [type.int]);

      it('should allow cast of pointer -> same pointer', function() {
        [v, c, e, s, u, f].forEach(function(x) {
          var p = type.Pointer(x),
              a,
              ia;
          assertCast(p, p, type.CAST_OK_EXACT);
          if (x.kind !== type.VOID) {
            a = type.Array(x, 2);
            ia = type.IncompleteArray(x);
            assertCast(p, a, type.CAST_OK_EXACT);
            assertCast(p, ia, type.CAST_OK_EXACT);
            assertCast(a, p, type.CAST_OK_EXACT);
            assertCast(a, a, type.CAST_OK_EXACT);
            assertCast(a, ia, type.CAST_OK_EXACT);
            assertCast(ia, p, type.CAST_OK_EXACT);
            assertCast(ia, a, type.CAST_OK_EXACT);
            assertCast(ia, ia, type.CAST_OK_EXACT);
          }
        });
      });

      it('should allow cast of non-void pointer <-> void pointer', function() {
        // Note that qualifiers are ignored in this case.
        var pv = type.Pointer(type.void);
        [c, e, s, u].forEach(function(x) {
          [0, C, CV, CVR, V, VR, R].forEach(function(q1) {
            [0, C, CV, CVR, V, VR, R].forEach(function(q2) {
              var p = type.Pointer(x),
                  a = type.Array(x, 2),
                  ia = type.IncompleteArray(x);
              assertCast(p.qualify(q1), pv.qualify(q2), type.CAST_OK_CONVERSION);
              assertCast(a, pv.qualify(q2), type.CAST_OK_CONVERSION);
              assertCast(ia, pv.qualify(q2), type.CAST_OK_CONVERSION);
              assertCast(pv.qualify(q1), p.qualify(q2), type.CAST_OK_CONVERSION);
              assertCast(pv.qualify(q1), a, type.CAST_OK_CONVERSION);
              assertCast(pv.qualify(q1), ia, type.CAST_OK_CONVERSION);
            });
          });
        });
      });

      it('should warn on cast of void* -> less qualified void*', function() {
        [0, C, CV, CVR, V, VR, R].forEach(function(q1) {
          var q1p = type.Pointer(v.qualify(q1));
          [0, C, CV, CVR, V, VR, R].forEach(function(q2) {
            if (!type.isLessQualified(q2, q1)) return;
            var q2p = type.Pointer(v.qualify(q2));
            assertCast(q1p, q2p, type.CAST_DISCARD_QUALIFIER);
          });
        });
      });

      it('should warn on cast of void pointer <-> function pointer', function() {
        // This is disallowed by the C spec, but seems to work without warning
        // in clang + gcc.
        var pv = type.Pointer(type.void),
            pf = type.Pointer(f);
        assertCast(pv, pf, type.CAST_VOID_POINTER_TO_FUNCTION_POINTER);
        assertCast(pf, pv, type.CAST_FUNCTION_POINTER_TO_VOID_POINTER);
      });

      it('should allow cast of pointer -> more qualified pointer', function() {
        [v, c, e, s, u, f].forEach(function(x) {
          var p = type.Pointer(x);
          [0, C, CV, CVR, V, VR, R].forEach(function(q1) {
            var q1p = p.qualify(q1);
            [0, C, CV, CVR, V, VR, R].forEach(function(q2) {
              if (!type.isLessQualified(q1, q2)) return;
              var q2p = p.qualify(q2);
              assertCast(q1p, q2p, type.CAST_OK_EXACT);
            });
          });
        });
      });

      it('should warn on cast of pointer -> less qualified pointee', function() {
        // Also warn if casting to a differently qualified pointee, e.g.
        // const void* => volatile void*
        [v, c, e, s, u].forEach(function(x) {
          [0, C, CV, CVR, V, VR, R].forEach(function(q1) {
            var q1p = type.Pointer(x.qualify(q1));
            [0, C, CV, CVR, V, VR, R].forEach(function(q2) {
              if (!type.isLessQualified(q2, q1)) return;
              var q2p = type.Pointer(x.qualify(q2));
              assertCast(q1p, q2p, type.CAST_DISCARD_QUALIFIER);
            });
          });
        });
      });

      it('should allow cast of pointer-like -> qualified pointee', function() {
        // Arrays cannot be qualified, so test unqualified arrays being cast to
        // qualified pointers.
        [c, e, s, u, f].forEach(function(x) {
          var p = type.Pointer(x),
              a = type.Array(x, 2),
              ia = type.IncompleteArray(x);
          [C, CV, CVR, V, VR, R].forEach(function(q) {
            var qp = type.Pointer(x.qualify(q));
            assertCast(a, qp, type.CAST_OK_EXACT);
            assertCast(ia, qp, type.CAST_OK_EXACT);
          });
        });
      });

      it('should warn on cast of pointer-like to incompatible pointer', function() {
        [c, e, s, u, f].forEach(function(x) {
          var xp = type.Pointer(x),
              xa = type.Array(x, 2),
              xia = type.IncompleteArray(x);
          [c, e, s, u, f].forEach(function(y) {
            if (x === y) return;

            var yp = type.Pointer(y),
                ya = type.Array(y, 2),
                yia = type.IncompleteArray(y);

            assertCast(xp, yp, type.CAST_INCOMPATIBLE_POINTERS);
            assertCast(xp, ya, type.CAST_INCOMPATIBLE_POINTERS);
            assertCast(xp, yia, type.CAST_INCOMPATIBLE_POINTERS);
            assertCast(xa, yp, type.CAST_INCOMPATIBLE_POINTERS);
            assertCast(xa, ya, type.CAST_INCOMPATIBLE_POINTERS);
            assertCast(xa, yia, type.CAST_INCOMPATIBLE_POINTERS);
            assertCast(xia, yp, type.CAST_INCOMPATIBLE_POINTERS);
            assertCast(xia, ya, type.CAST_INCOMPATIBLE_POINTERS);
            assertCast(xia, yia, type.CAST_INCOMPATIBLE_POINTERS);
          });
        });
      });

      it('should warn on cast between differently-qualified pointees', function() {
        [0, C, V, R, CV, CR, VR, CVR].forEach(function (q1) {
          var q1p = type.Pointer(type.Pointer(type.char.qualify(q1)));
          [0, C, V, R, CV, CR, VR, CVR].forEach(function (q2) {
            if (q1 === q2) return;
            var q2p = type.Pointer(type.Pointer(type.char.qualify(q2)));
            assertCast(q1p, q2p, type.CAST_INCOMPATIBLE_POINTERS);
          });
        });
      });

      it('should warn on cast of pointer-like -> integral', function() {
        [v, c, e, s, u, f].forEach(function(x) {
          assertCast(type.Pointer(x), type.int, type.CAST_POINTER_TO_INT);
          if (x.kind !== type.VOID) {
            assertCast(type.Array(x, 2), type.int, type.CAST_POINTER_TO_INT);
            assertCast(type.IncompleteArray(x), type.int, type.CAST_POINTER_TO_INT);
          }
        });
      });

      it('should fail on cast of pointer-like -> float', function() {
        [v, c, e, s, u, f].forEach(function(x) {
          var p = type.Pointer(x);
          assertCast(p, type.float, type.CAST_ERROR);
          assertCast(p, type.double, type.CAST_ERROR);
          if (x.kind !== type.VOID) {
            assertCast(type.Array(x, 2), type.float, type.CAST_ERROR);
            assertCast(type.Array(x, 2), type.double, type.CAST_ERROR);
            assertCast(type.IncompleteArray(x), type.float, type.CAST_ERROR);
            assertCast(type.IncompleteArray(x), type.double, type.CAST_ERROR);
          }
        });
      });

      it('should fail on cast of pointer-like -> anything else', function() {
        [v, c, e, s, u, f].forEach(function(x) {
          var p = type.Pointer(x);
          [v, e, s, u, f].forEach(function(to) {
            assertCast(p, to, type.CAST_ERROR);
            if (x.kind !== type.VOID) {
              assertCast(type.Array(x, 2), to, type.CAST_ERROR);
              assertCast(type.IncompleteArray(x), to, type.CAST_ERROR);
            }
          });
        });
      });
    });

    describe('Record', function() {
      var s = type.Record('s', [type.Field('f', type.int, 0)]),
          s2 = type.Record('s2', [type.Field('f', type.int, 0)]),
          u = type.Record('s', [type.Field('f', type.int, 0)], type.UNION);

      it('should allow cast to same record', function() {
        assertCast(s, s, type.CAST_OK_EXACT);
        assertCast(u, u, type.CAST_OK_EXACT);
      });

      it('should fail on cast of union <-> struct of same tag', function() {
        // It's not possible to do this in C, but regardless, the cast
        // shouldn't succeed.
        assertCast(u, s, type.CAST_ERROR);
        assertCast(s, u, type.CAST_ERROR);
      });

      it('should fail on cast of struct -> different tag', function() {
        assertCast(s, s2, type.CAST_ERROR);
      });

      it('should fail on cast of record -> anything else', function() {
        assertCast(s, type.void, type.CAST_ERROR);
        assertCast(s, type.char, type.CAST_ERROR);
        assertCast(s, type.int, type.CAST_ERROR);
        assertCast(s, type.Pointer(type.int), type.CAST_ERROR);
        assertCast(s, type.Enum('e'), type.CAST_ERROR);
        assertCast(s, type.Function(type.int, [type.int]), type.CAST_ERROR);
        assertCast(s, type.Array(type.int, 2), type.CAST_ERROR);
        assertCast(s, type.IncompleteArray(type.int), type.CAST_ERROR);
      });
    });

    describe('Enum', function() {
      var e = type.Enum('E'),
          e2 = type.Enum('E2');

      it('should allow cast of enum -> same enum', function() {
        assertCast(e, e, type.CAST_OK_EXACT);
      });

      it('should warn on cast of enum -> another enum', function() {
        // C allows this, but I'd rather warn.
        assertCast(e, e2, type.CAST_DIFFERENT_ENUMS);
        assertCast(e2, e, type.CAST_DIFFERENT_ENUMS);
      });

      it('should allow cast of enum -> integral', function() {
        assertCast(e, type.bool, type.CAST_OK_CONVERSION);
        assertCast(e, type.char, type.CAST_OK_CONVERSION);
        assertCast(e, type.short, type.CAST_OK_CONVERSION);
        assertCast(e, type.int, type.CAST_OK_CONVERSION);
        assertCast(e, type.long, type.CAST_OK_CONVERSION);
        assertCast(e, type.longlong, type.CAST_OK_CONVERSION);
        assertCast(e, type.uchar, type.CAST_OK_CONVERSION);
        assertCast(e, type.ushort, type.CAST_OK_CONVERSION);
        assertCast(e, type.uint, type.CAST_OK_CONVERSION);
        assertCast(e, type.ulong, type.CAST_OK_CONVERSION);
        assertCast(e, type.ulonglong, type.CAST_OK_CONVERSION);
      });

      it('should fail on cast of enum -> float', function() {
        assertCast(e, type.float, type.CAST_ERROR);
        assertCast(e, type.double, type.CAST_ERROR);
      });

      it('should fail on cast of enum -> anything else', function() {
        assertCast(e, type.void, type.CAST_ERROR);
        assertCast(e, type.Pointer(type.int), type.CAST_ERROR);
        assertCast(e, type.Record('s', [type.Field('f', type.int, 0)]), type.CAST_ERROR);
        assertCast(e, type.Function(type.int, [type.int]), type.CAST_ERROR);
        assertCast(e, type.Array(type.int, 2), type.CAST_ERROR);
        assertCast(e, type.IncompleteArray(type.int), type.CAST_ERROR);
      });
    });

    describe('Function', function() {
      it('should fail to cast function -> anything', function() {
        // Bare functions cannot even be specified in C (only C++). Referencing
        // a function in C is typed as a function pointer.
        var f = type.Function(type.int, [type.int]);
        assertCast(f, f, type.CAST_ERROR);
        assertCast(f, type.void, type.CAST_ERROR);
        assertCast(f, type.int, type.CAST_ERROR);
        assertCast(f, type.Enum('e'), type.CAST_ERROR);
        assertCast(f, type.Pointer(type.int), type.CAST_ERROR);
        assertCast(f, type.Record('s', [type.Field('f', type.int, 0)]), type.CAST_ERROR);
        assertCast(f, type.Array(type.int, 2), type.CAST_ERROR);
        assertCast(f, type.IncompleteArray(type.int), type.CAST_ERROR);
      });
    });

    describe('Typedef', function() {
      it('should allow cast of pointer -> same pointer w/ typedef', function() {
        var Pc = type.Pointer(type.char),
            PPc = type.Pointer(Pc),
            t = type.Typedef('t', Pc),
            Pt = type.Pointer(t);
        assertCast(Pc, t, type.CAST_OK_EXACT);
        assertCast(t, Pc, type.CAST_OK_EXACT);
        assertCast(PPc, Pt, type.CAST_OK_EXACT);
        assertCast(Pt, PPc, type.CAST_OK_EXACT);
      });

      it('should allow qualifiers to propagate though typedef', function() {
        var Kt = type.Typedef('t', type.char, type.CONST),
            Kc = type.char.qualify(type.CONST),
            VKt = type.Typedef('t2', Kt, type.VOLATILE),
            KVc = type.char.qualify(type.CONST | type.VOLATILE);
        assertCast(type.Pointer(Kt), type.Pointer(Kc), type.CAST_OK_EXACT);
        assertCast(type.Pointer(Kc), type.Pointer(Kt), type.CAST_OK_EXACT);
        assertCast(type.Pointer(VKt), type.Pointer(KVc), type.CAST_OK_EXACT);
        assertCast(type.Pointer(KVc), type.Pointer(VKt), type.CAST_OK_EXACT);
      });
    });
  });

  describe('BestViable', function() {
    function assertBestViable(fns, argTypes, expected) {
      var actual = type.getBestViableFunction(fns, argTypes);
      var aSpell = actual >= 0 ? spell(fns[actual]) : 'null';
      var eSpell = expected >= 0 ? spell(fns[expected]) : 'null';
      var msg = 'Expected: "' + eSpell + '" (' + expected + ') ' +
                'Actual: "' + aSpell + '" (' + actual + ').';
      assert.strictEqual(actual, expected, msg);
    }

    it('should work if there is 1 overload w/ an exact match', function() {
      var fn0 = type.Function(type.void, [type.int]),
          fns = [fn0];
      assertBestViable(fns, [type.int], 0);
    });

    it('should work if there are 2 overloads w/ an exact match', function() {
      var Pi = type.Pointer(type.int),
          fn0 = type.Function(type.void, [type.int]),
          fn1 = type.Function(type.void, [Pi]),
          fns = [fn0, fn1];
      assertBestViable(fns, [type.int], 0);
      assertBestViable(fns, [Pi], 1);
    });

    it('should prefer an exact match over a promotion', function() {
      var fn0 = type.Function(type.void, [type.int]),
          fn1 = type.Function(type.void, [type.short]),
          fns = [fn0, fn1];
      assertBestViable(fns, [type.int], 0);
      assertBestViable(fns, [type.short], 1);
    });

    it('should prefer a int/float promotion over conversion', function() {
      var fn0 = type.Function(type.void, [type.int]),
          fn1 = type.Function(type.void, [type.double]),
          fns = [fn0, fn1];
      assertBestViable(fns, [type.char], 0);
      assertBestViable(fns, [type.float], 1);
    });

    it('should choose a promotion if it is available', function() {
      var fn0 = type.Function(type.void, [type.int]),
          fn1 = type.Function(type.void, [type.Pointer(type.int)]),
          fns = [fn0, fn1];
      assertBestViable(fns, [type.short], 0);
    });

    it('should prefer an exact match over a conversion', function() {
      var e = type.Enum('e'),
          fn0 = type.Function(type.void, [e]),
          fn1 = type.Function(type.void, [type.int]),
          fns = [fn0, fn1];
      assertBestViable(fns, [e], 0);
    });

    it('should choose a conversion if it is available', function() {
      var e = type.Enum('e'),
          fn0 = type.Function(type.void, [type.int]),
          fns = [fn0];
      assertBestViable(fns, [e], 0);
    });

    it('should work with multiple arguments', function() {
      var i = type.int,
          c = type.char,
          Pi = type.Pointer(i),
          fn0 = type.Function(type.void, [i, c]),
          fn1 = type.Function(type.void, [i, Pi]),
          fns = [fn0, fn1];
      assertBestViable(fns, [i, c], 0);
      assertBestViable(fns, [i, Pi], 1);
    });

    it('should ignore functions that aren\'t viable', function() {
      var i = type.int,
          c = type.char,
          fn0 = type.Function(type.void, [i, i]),
          fn1 = type.Function(type.void, [i, i, i, i]),  // Not viable
          fns = [fn0, fn1];
      assertBestViable(fns, [i, i], 0);
      assertBestViable(fns, [i, c], 0);
    });

    it('should allow multiple promotions/conversions', function() {
      var i = type.int,
          Pv = type.Pointer(type.void),
          Pi = type.Pointer(i),
          c = type.char,
          fn0 = type.Function(type.void, [Pi, i]),
          fns = [fn0];
      assertBestViable(fns, [Pv, c], 0);
    });

    it('should allow multiple promotions/conversions', function() {
      var i = type.int,
          Pv = type.Pointer(type.void),
          Pi = type.Pointer(i),
          c = type.char,
          fn0 = type.Function(type.void, [Pi, i]),
          fns = [fn0];
      assertBestViable(fns, [Pv, c], 0);
    });

    it('should fail if there are no viable functions', function() {
      var i = type.int,
          fn0 = type.Function(type.void, [i, i]),
          fns = [fn0];
      assertBestViable(fns, [], -1);
      assertBestViable(fns, [i], -1);
    });

    it('should fail if no function is best', function() {
      var i = type.int,
          c = type.char,
          fn0 = type.Function(type.void, [c, i]),
          fn1 = type.Function(type.void, [i, c]),
          fns = [fn0, fn1];
      assertBestViable(fns, [c, c], -1);
    });
  });
});
