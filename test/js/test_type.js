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

var type = require('../../src/js/naclbind').type;
var assertTypesEqual = require('./equals').assertTypesEqual;
var chai = require('chai');
var assert = chai.assert;
var spell = type.getSpelling;
var qual = type.describeQualifier;
var canon = type.getCanonical;
var viable = type.getBestViableFunction;
var C = type.CONST;
var CV = type.CONST | type.VOLATILE;
var CR = type.CONST | type.RESTRICT;
var CVR = type.CONST | type.VOLATILE | type.RESTRICT;
var V = type.VOLATILE;
var VR = type.VOLATILE | type.RESTRICT;
var R = type.RESTRICT;

chai.config.includeStack = true;

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
          ];
      var Q = [0, C, V, R, CV, CR, VR, CVR];
      var NotOK = [];

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
            type.Record('s', 4, type.STRUCT, badCv);
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

    describe('FunctionNoProto', function() {
      it('should throw creating functions with bad resultType', function() {
        [undefined, null, 100, 'int'].forEach(function(badType) {
          assert.throws(function() {
            type.FunctionNoProto(badType);
          }, /resultType/);
        });
      });

      it('should throw creating functions that return arrays', function() {
        assert.throws(function() {
          type.FunctionNoProto(type.Array(type.int, 2));
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
      it('should allow access to fields', function() {
        var r = type.Record('foo', 4);
        r.$addField('f', type.int, 0);
        r.$addField('g', type.float, 4);

        assert.strictEqual(r.$fields[0].$name, 'f');
        assertTypesEqual(r.$fields[0].$type, type.int);
        assert.strictEqual(r.$fields[0].$offset, 0);

        assert.strictEqual(r.$fields[1].$name, 'g');
        assertTypesEqual(r.$fields[1].$type, type.float);
        assert.strictEqual(r.$fields[1].$offset, 4);
      });

      it('should throw creating a record with bad name', function() {
        // Name should be string or null (for anonymous).
        [undefined, 100].forEach(function(badTag) {
          assert.throws(function() {
            type.Record(badTag, 4);
          });
        });
      });

      it('should throw creating a record with bad fields', function() {
        // Field name must be string or null (for anonymous)
        assert.throws(function() {
          var r = type.Record('foo', 4);
          r.addField(1234, type.int, 0);
        });

        // Field must have valid type
        assert.throws(function() {
          var r = type.Record('foo', 0);
          r.addField('f', null, 0);
        });

        // Field offset must be number
        assert.throws(function() {
          type.Record('foo', 4);
          r.addField('f', type.int, 'none');
        });
      });

      it('should throw creating a record with bad struct/union', function() {
        // Should be type.STRUCT/type.UNION (bool)
        assert.throws(function() {
          type.Record('foo', 4, 'struct');
        });

        assert.throws(function() {
          type.Record('foo', 4, null);
        });
      });
    });
  });

  describe('Size', function() {
    it('should have the correct size for primitives', function() {
      assert.strictEqual(type.void.$size, 0);
      assert.strictEqual(type.bool.$size, 1);
      assert.strictEqual(type.char.$size, 1);
      assert.strictEqual(type.uchar.$size, 1);
      assert.strictEqual(type.ushort.$size, 2);
      assert.strictEqual(type.uint.$size, 4);
      assert.strictEqual(type.ulong.$size, 4);
      assert.strictEqual(type.ulonglong.$size, 8);
      assert.strictEqual(type.schar.$size, 1);
      assert.strictEqual(type.wchar.$size, 4);
      assert.strictEqual(type.short.$size, 2);
      assert.strictEqual(type.int.$size, 4);
      assert.strictEqual(type.long.$size, 4);
      assert.strictEqual(type.longlong.$size, 8);
      assert.strictEqual(type.float.$size, 4);
      assert.strictEqual(type.double.$size, 8);
      assert.strictEqual(type.longdouble.$size, 10);
    });

    it('should have the correct size for pointers', function() {
      assert.strictEqual(type.Pointer(type.void).$size, 4);
      assert.strictEqual(type.Pointer(type.int).$size, 4);
      assert.strictEqual(type.Pointer(type.longlong).$size, 4);
      assert.strictEqual(type.Pointer(type.Function(type.void, [])).$size, 4);
    });

    it('should have the correct size for records', function() {
      var s1 = type.Record('s1', 12, type.STRUCT);
      var s2 = type.Record('s2', 4, type.UNION);

      assert.strictEqual(s1.$size, 12);
      assert.strictEqual(s2.$size, 4);
    });

    it('should have the correct size for enums', function() {
      var e1 = type.Enum('e1');

      assert.strictEqual(e1.$size, 4);
    });

    it('should have the correct size for typedefs', function() {
      assert.strictEqual(type.Typedef('t1', type.void).$size, 0);
      assert.strictEqual(type.Typedef('t2', type.short).$size, 2);
      assert.strictEqual(type.Typedef('t3', type.int).$size, 4);
      assert.strictEqual(type.Typedef('t4', type.long).$size, 4);
      assert.strictEqual(type.Typedef('t5', type.longlong).$size, 8);
      assert.strictEqual(type.Typedef('t6', type.Pointer(type.void)).$size, 4);
      assert.strictEqual(type.Typedef('t7', type.Enum('e1')).$size, 4);
      assert.strictEqual(type.Typedef('t8', type.Record('s1', 12)).$size, 12);
      assert.strictEqual(type.Typedef('t9', type.Typedef('t9', type.int)).$size,
                         4);
    });

    it('should have no size for a function', function() {
      assert.strictEqual(type.Function(type.void, []).$size, -1);
      assert.strictEqual(type.FunctionNoProto(type.void).$size, -1);
      assert.strictEqual(type.FunctionUntyped().$size, -1);
    });

    it('should have size for an array', function() {
      assert.strictEqual(type.Array(type.int, 10).$size, 40);
      assert.strictEqual(type.Array(type.Pointer(type.void), 5).$size, 20);
      assert.strictEqual(type.Array(type.Typedef('t1', type.char), 5).$size, 5);
    });

    it('should have size for an incomplete array', function() {
      assert.strictEqual(type.IncompleteArray(type.int).$size, 4);
      assert.strictEqual(type.IncompleteArray(type.Pointer(type.void)).$size, 4);
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
      assert.strictEqual(spell(type.longdouble), 'long double');
      assert.strictEqual(spell(type.wchar), 'wchar_t');
    });

    it('should correctly spell qualified primitive types', function() {
      var cv = type.CONST | type.VOLATILE;
      var cvr = type.CONST | type.VOLATILE | type.RESTRICT;
      assert.strictEqual(spell(type.Void(type.CONST)), 'const void');
      assert.strictEqual(spell(type.char.$qualify(type.CONST)), 'const char');
      assert.strictEqual(spell(type.int.$qualify(type.CONST)), 'const int');
      assert.strictEqual(spell(type.int.$qualify(cv)), 'const volatile int');
      assert.strictEqual(spell(type.float.$qualify(cvr)),
                   'const volatile restrict float');
    });

    it('should correctly spell record types', function() {
      var s = type.Record('s', 4);
      var u = type.Record('u', 4, type.UNION);
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
      var s = type.Record('myStruct', 4);
      var e = type.Enum('myEnum');
      var t = type.Typedef('myTypedef', type.int);
      var f = type.Function(type.void, [type.int]);
      assert.strictEqual(spell(type.Pointer(type.void)), 'void *');
      assert.strictEqual(spell(type.Pointer(type.int)), 'int *');
      assert.strictEqual(spell(type.Pointer(s)), 'struct myStruct *');
      assert.strictEqual(spell(type.Pointer(e)), 'enum myEnum *');
      assert.strictEqual(spell(type.Pointer(t)), 'myTypedef *');
      assert.strictEqual(spell(type.Pointer(f)), 'void (*)(int)');
      assert.strictEqual(spell(type.Pointer(type.Pointer(type.int))), 'int **');
    });

    it('should correctly spell qualified pointer types', function() {
      var Kc = type.char.$qualify(type.CONST);
      var PKc = type.Pointer(Kc);
      var PKPc = type.Pointer(type.Pointer(type.char, type.CONST));
      var PKt = type.Pointer(type.Typedef('foo', type.char, type.CONST));
      var PKPKc = type.Pointer(type.Pointer(Kc, type.CONST));
      var PKVi = type.Pointer(type.int.$qualify(type.CONST | type.VOLATILE));
      assert.strictEqual(spell(PKc), 'const char *');
      assert.strictEqual(spell(PKPc), 'char *const *');
      assert.strictEqual(spell(PKt), 'const foo *');
      assert.strictEqual(spell(PKPKc), 'const char *const *');
      assert.strictEqual(spell(PKVi), 'const volatile int *');
    });

    it('should correctly spell function types', function() {
      var PKc = type.Pointer(type.char.$qualify(type.CONST));
      var f1 = type.Function(type.int, [type.int, type.int]);
      var f2 = type.Function(type.int, [PKc], type.VARIADIC);
      var f3 = type.Pointer(type.Function(type.void, [type.int]));
      var f4 = type.Function(f3, [type.int, f3]);
      var f5 = type.Function(type.void, []);
      var f6 = type.FunctionNoProto(type.void);
      var f7 = type.Pointer(type.FunctionNoProto(type.void));
      assert.strictEqual(spell(f1), 'int (int, int)');
      assert.strictEqual(spell(f2), 'int (const char *, ...)');
      assert.strictEqual(spell(f4), 'void (*(int, void (*)(int)))(int)');
      assert.strictEqual(spell(f4, 'signal'),
                   'void (*signal(int, void (*)(int)))(int)');
      assert.strictEqual(spell(f5), 'void (void)');
      assert.strictEqual(spell(f6), 'void ()');
      assert.strictEqual(spell(f7), 'void (*)()');
    });

    it('should handle spelling precedence', function() {
      var IA = type.IncompleteArray;
      var P = type.Pointer;
      var F = type.Function;
      var i = type.int;
      var A_PFiiE = IA(P(F(i, [i])));
      var PA_PFiiE = P(IA(P(F(i, [i]))));
      var PFPFPivEiE = P(F(P(F(P(i), [])), [i]));
      assert.strictEqual(spell(A_PFiiE), 'int (*[])(int)');
      assert.strictEqual(spell(PA_PFiiE), 'int (*(*)[])(int)');
      assert.strictEqual(spell(PA_PFiiE, 'foo'), 'int (*(*foo)[])(int)');
      assert.strictEqual(spell(PFPFPivEiE), 'int *(*(*)(int))(void)');
    });

    it('should throw when given a type with unknown kind', function() {
      var kind = 1000;
      var t = type.Type(kind);
      assert.throws(function() {
        spell(t);
      }, /kind/);
    });
  });

  describe('Canonical', function() {
    it('should ignore types with typedefs', function() {
      var v = type.void;
      var i = type.int;
      var Pc = type.Pointer(type.char);
      var A2_c = type.Array(type.char, 2);
      var A_c = type.IncompleteArray(type.char);
      var s = type.Record('s', 4);
      var e = type.Enum('e');
      var FiiE = type.Function(type.int, [type.int]);
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
        type.Record('s', 4),
        type.Enum('e'),
        type.Function(type.int, [type.int])
      ];

      types.forEach(function(t) {
        var typedef = type.Typedef('t', t);
        assertTypesEqual(canon(typedef), t);
      });
    });

    it('should reduce typedefs of child types', function() {
      var t = type.Typedef('t', type.char);
      var tt = type.Typedef('tt', t);

      [t, tt].forEach(function(x) {
        var Px = type.Pointer(x);
        var A2_x = type.Array(x, 2);
        var A_x = type.IncompleteArray(x);
        var FvxE = type.Function(type.void, [x]);
        var FxvE = type.Function(x, []);

        assertTypesEqual(canon(x), type.char);
        assertTypesEqual(canon(Px), type.Pointer(type.char));
        assertTypesEqual(canon(A2_x), type.Array(type.char, 2));
        assertTypesEqual(canon(A_x), type.IncompleteArray(type.char));
        assertTypesEqual(canon(FvxE), type.Function(type.void, [type.char]));
        assertTypesEqual(canon(FxvE), type.Function(type.char, []));
      });
    });

    it('should combine typedef qualifiers', function() {
      var Kc = type.char.$qualify(type.CONST);
      var Kt = type.Typedef('Kt', Kc, type.CONST);
      var Vt = type.Typedef('Vt', Kc, type.VOLATILE);

      assertTypesEqual(canon(Kt), Kc);  // Extra const is ignored.
      assertTypesEqual(canon(Vt),
                       type.char.$qualify(type.CONST | type.VOLATILE));
    });
  });

  describe('Cast', function() {
    function spellTypedef(t) {
      if (t.$kind === type.TYPEDEF) {
        return 'typedef of ' + spellTypedef(t.$alias);
      }
      return spell(t);
    }

    function assertCastHelper(from, to, expected) {
      var actual = from.$canCastTo(to);
      var msg = 'Cast from "' + spellTypedef(from) + '" -> "' +
                                spellTypedef(to) + '": ' +
                'expected: ' + expected + ' actual: ' + actual;
      assert.strictEqual(actual, expected, msg);
    }

    function assertCast(from, to, expected) {
      var tfrom = type.Typedef('from', from);
      var ttfrom = type.Typedef('fromfrom', tfrom);
      var tto = type.Typedef('to', to);
      var ttto = type.Typedef('toto', tto);

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
        var e = type.Enum('e');
        var s = type.Record('s', 4);
        var u = type.Record('s', 4, type.UNION);
        var f = type.Function(type.void, [type.int]);
        var fn = type.FunctionNoProto(type.void);
        var fu = type.FunctionUntyped();
        var fp = type.Pointer(f);
        var p = type.Pointer(type.void);
        var a = type.Array(type.char, 2);
        var ia = type.IncompleteArray(type.char);
        assertCast(type.void, e, type.CAST_ERROR);
        assertCast(type.void, s, type.CAST_ERROR);
        assertCast(type.void, u, type.CAST_ERROR);
        assertCast(type.void, f, type.CAST_ERROR);
        assertCast(type.void, fn, type.CAST_ERROR);
        assertCast(type.void, fu, type.CAST_ERROR);
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
        assertCast(type.double, type.longdouble, type.CAST_OK_PROMOTION);
        assertCast(type.float, type.longdouble, type.CAST_OK_PROMOTION);
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
        assertCast(type.longdouble, type.float, type.CAST_TRUNCATE);
        assertCast(type.longdouble, type.double, type.CAST_TRUNCATE);
      });

      it('should warn on cast of integral -> pointer-like', function() {
        var c = type.char;
        var v = type.void;
        var p = type.Pointer(v);
        var a = type.Array(c, 2);
        var ia = type.IncompleteArray(c);
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
        var c = type.char;
        var v = type.void;
        var p = type.Pointer(v);
        var a = type.Array(c, 2);
        var ia = type.IncompleteArray(c);
        [p, a, ia].forEach(function(x) {
          assertCast(type.float, x, type.CAST_ERROR);
          assertCast(type.double, x, type.CAST_ERROR);
          assertCast(type.longdouble, x, type.CAST_ERROR);
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
        assertCast(type.longdouble, e, type.CAST_ERROR);
      });

      it('should fail to cast numeric -> record, void, function', function() {
        var s = type.Record('s', 4);
        var u = type.Record('s', 4, type.UNION);
        var v = type.void;
        var f = type.Function(type.void, [type.int]);
        var fn = type.FunctionNoProto(type.void);
        var fu = type.FunctionUntyped();
        [s, u, v, f, fn, fu].forEach(function(x) {
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
          assertCast(type.longdouble, x, type.CAST_ERROR);
        });
      });
    });

    describe('Pointer', function() {
      var v = type.void;
      var c = type.char;
      var e = type.Enum('e');
      var s = type.Record('s', 4);
      var u = type.Record('s', 4, type.UNION);
      var f = type.Function(type.void, [type.int]);
      var fn = type.FunctionNoProto(type.void);
      var fu = type.FunctionUntyped();

      it('should allow cast of pointer -> same pointer', function() {
        [v, c, e, s, u, f, fn, fu].forEach(function(x) {
          var p = type.Pointer(x);
          var a;
          var ia;
          assertCast(p, p, type.CAST_OK_EXACT);
          if (x.$kind !== type.VOID) {
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

      it('should warn on cast of fun* <-> fun* w/ no proto', function() {
        var pf = type.Pointer(f);
        var pfn = type.Pointer(fn);
        var af = type.Array(f, 2);
        var afn = type.Array(fn, 2);
        var iaf = type.IncompleteArray(f);
        var iafn = type.IncompleteArray(fn);

        [pf, af, iaf].forEach(function(from) {
          [pfn, afn, iafn].forEach(function(to) {
            assertCast(from, to, type.CAST_FUNCTION_POINTER_NOPROTO);
            assertCast(to, from, type.CAST_FUNCTION_POINTER_NOPROTO);
          });
        });
      });

      it('should allow cast of untyped fun <-> any other fun', function() {
        var pf = type.Pointer(f);
        var pfn = type.Pointer(fn);
        var pfu = type.Pointer(fu);
        var af = type.Array(f, 2);
        var afn = type.Array(fn, 2);
        var afu = type.Array(fu, 2);
        var iaf = type.IncompleteArray(f);
        var iafn = type.IncompleteArray(fn);
        var iafu = type.IncompleteArray(fu);

        [pfu, afu, iafu].forEach(function(from) {
          [pf, af, iaf, pfn, afn, iafn].forEach(function(to) {
            assertCast(from, to, type.CAST_OK_EXACT);
            assertCast(to, from, type.CAST_OK_EXACT);
          });
        });
      });

      it('should allow cast of non-void pointer <-> void pointer', function() {
        // Note that qualifiers are ignored in this case.
        var pv = type.Pointer(type.void);
        [c, e, s, u].forEach(function(x) {
          [0, C, CV, CVR, V, VR, R].forEach(function(q1) {
            [0, C, CV, CVR, V, VR, R].forEach(function(q2) {
              var p = type.Pointer(x);
              var a = type.Array(x, 2);
              var ia = type.IncompleteArray(x);
              assertCast(p.$qualify(q1), pv.$qualify(q2), type.CAST_OK_CONVERSION);
              assertCast(a, pv.$qualify(q2), type.CAST_OK_CONVERSION);
              assertCast(ia, pv.$qualify(q2), type.CAST_OK_CONVERSION);
              assertCast(pv.$qualify(q1), p.$qualify(q2), type.CAST_OK_CONVERSION);
              assertCast(pv.$qualify(q1), a, type.CAST_OK_CONVERSION);
              assertCast(pv.$qualify(q1), ia, type.CAST_OK_CONVERSION);
            });
          });
        });
      });

      it('should warn on cast of void* -> less qualified void*', function() {
        [0, C, CV, CVR, V, VR, R].forEach(function(q1) {
          var q1p = type.Pointer(v.$qualify(q1));
          [0, C, CV, CVR, V, VR, R].forEach(function(q2) {
            if (!type.isLessQualified(q2, q1)) return;
            var q2p = type.Pointer(v.$qualify(q2));
            assertCast(q1p, q2p, type.CAST_DISCARD_QUALIFIER);
          });
        });
      });

      it('should warn on cast of void pointer <-> function pointer', function() {
        // This is disallowed by the C spec, but seems to work without warning
        // in clang + gcc.
        var pv = type.Pointer(type.void);
        var pf = type.Pointer(f);
        var pfn = type.Pointer(fn);
        var pfu = type.Pointer(fu);
        assertCast(pv, pf, type.CAST_FUNCTION_POINTER_VOID_POINTER);
        assertCast(pf, pv, type.CAST_FUNCTION_POINTER_VOID_POINTER);
        assertCast(pv, pfn, type.CAST_FUNCTION_POINTER_VOID_POINTER);
        assertCast(pfn, pv, type.CAST_FUNCTION_POINTER_VOID_POINTER);
        assertCast(pv, pfu, type.CAST_FUNCTION_POINTER_VOID_POINTER);
        assertCast(pfu, pv, type.CAST_FUNCTION_POINTER_VOID_POINTER);
      });

      it('should allow cast of pointer -> more qualified pointer', function() {
        [v, c, e, s, u, f, fn, fu].forEach(function(x) {
          var p = type.Pointer(x);
          [0, C, CV, CVR, V, VR, R].forEach(function(q1) {
            var q1p = p.$qualify(q1);
            [0, C, CV, CVR, V, VR, R].forEach(function(q2) {
              if (!type.isLessQualified(q1, q2)) return;
              var q2p = p.$qualify(q2);
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
            var q1p = type.Pointer(x.$qualify(q1));
            [0, C, CV, CVR, V, VR, R].forEach(function(q2) {
              if (!type.isLessQualified(q2, q1)) return;
              var q2p = type.Pointer(x.$qualify(q2));
              assertCast(q1p, q2p, type.CAST_DISCARD_QUALIFIER);
            });
          });
        });
      });

      it('should allow cast of pointer-like -> qualified pointee', function() {
        // Arrays cannot be qualified, so test unqualified arrays being cast to
        // qualified pointers.
        [c, e, s, u, f, fn, fu].forEach(function(x) {
          var p = type.Pointer(x);
          var a = type.Array(x, 2);
          var ia = type.IncompleteArray(x);
          [C, CV, CVR, V, VR, R].forEach(function(q) {
            var qp = type.Pointer(x.$qualify(q));
            assertCast(a, qp, type.CAST_OK_EXACT);
            assertCast(ia, qp, type.CAST_OK_EXACT);
          });
        });
      });

      it('should warn on cast of pointer-like to incompatible pointer', function() {
        [c, e, s, u, f].forEach(function(x) {
          var xp = type.Pointer(x);
          var xa = type.Array(x, 2);
          var xia = type.IncompleteArray(x);
          [c, e, s, u, f].forEach(function(y) {
            if (x === y) return;

            var yp = type.Pointer(y);
            var ya = type.Array(y, 2);
            var yia = type.IncompleteArray(y);

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
          var q1p = type.Pointer(type.Pointer(type.char.$qualify(q1)));
          [0, C, V, R, CV, CR, VR, CVR].forEach(function (q2) {
            if (q1 === q2) return;
            var q2p = type.Pointer(type.Pointer(type.char.$qualify(q2)));
            assertCast(q1p, q2p, type.CAST_INCOMPATIBLE_POINTERS);
          });
        });
      });

      it('should warn on cast of pointer-like -> integral', function() {
        [v, c, e, s, u, f, fn, fu].forEach(function(x) {
          assertCast(type.Pointer(x), type.int, type.CAST_POINTER_TO_INT);
          if (x.$kind !== type.VOID) {
            assertCast(type.Array(x, 2), type.int, type.CAST_POINTER_TO_INT);
            assertCast(type.IncompleteArray(x), type.int, type.CAST_POINTER_TO_INT);
          }
        });
      });

      it('should fail on cast of pointer-like -> float', function() {
        [v, c, e, s, u, f, fn, fu].forEach(function(x) {
          var p = type.Pointer(x);
          assertCast(p, type.float, type.CAST_ERROR);
          assertCast(p, type.double, type.CAST_ERROR);
          assertCast(p, type.longdouble, type.CAST_ERROR);
          if (x.$kind !== type.VOID) {
            assertCast(type.Array(x, 2), type.float, type.CAST_ERROR);
            assertCast(type.Array(x, 2), type.double, type.CAST_ERROR);
            assertCast(type.Array(x, 2), type.longdouble, type.CAST_ERROR);
            assertCast(type.IncompleteArray(x), type.float, type.CAST_ERROR);
            assertCast(type.IncompleteArray(x), type.double, type.CAST_ERROR);
            assertCast(type.IncompleteArray(x), type.longdouble, type.CAST_ERROR);
          }
        });
      });

      it('should fail on cast of pointer-like -> anything else', function() {
        [v, c, e, s, u, f, fn, fu].forEach(function(x) {
          var p = type.Pointer(x);
          [v, e, s, u, f].forEach(function(to) {
            assertCast(p, to, type.CAST_ERROR);
            if (x.$kind !== type.VOID) {
              assertCast(type.Array(x, 2), to, type.CAST_ERROR);
              assertCast(type.IncompleteArray(x), to, type.CAST_ERROR);
            }
          });
        });
      });
    });

    describe('Record', function() {
      var s = type.Record('s', 4);
      var s2 = type.Record('s2', 4);
      var u = type.Record('s', 4, type.UNION);

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
        assertCast(s, type.FunctionNoProto(type.int), type.CAST_ERROR);
        assertCast(s, type.FunctionUntyped(), type.CAST_ERROR);
        assertCast(s, type.Array(type.int, 2), type.CAST_ERROR);
        assertCast(s, type.IncompleteArray(type.int), type.CAST_ERROR);
      });
    });

    describe('Enum', function() {
      var e = type.Enum('E');
      var e2 = type.Enum('E2');

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
        assertCast(e, type.longdouble, type.CAST_ERROR);
      });

      it('should fail on cast of enum -> anything else', function() {
        assertCast(e, type.void, type.CAST_ERROR);
        assertCast(e, type.Pointer(type.int), type.CAST_ERROR);
        assertCast(e, type.Record('s', 4), type.CAST_ERROR);
        assertCast(e, type.Function(type.int, [type.int]), type.CAST_ERROR);
        assertCast(e, type.FunctionNoProto(type.int), type.CAST_ERROR);
        assertCast(e, type.FunctionUntyped(), type.CAST_ERROR);
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
        assertCast(f, type.Record('s', 4), type.CAST_ERROR);
        assertCast(f, type.Array(type.int, 2), type.CAST_ERROR);
        assertCast(f, type.IncompleteArray(type.int), type.CAST_ERROR);
      });
    });

    describe('FunctionNoProto', function() {
      it('should fail to cast function -> anything', function() {
        // Bare functions cannot even be specified in C (only C++). Referencing
        // a function in C is typed as a function pointer.
        var f = type.FunctionNoProto(type.int);
        assertCast(f, f, type.CAST_ERROR);
        assertCast(f, type.void, type.CAST_ERROR);
        assertCast(f, type.int, type.CAST_ERROR);
        assertCast(f, type.Enum('e'), type.CAST_ERROR);
        assertCast(f, type.Pointer(type.int), type.CAST_ERROR);
        assertCast(f, type.Record('s', 4), type.CAST_ERROR);
        assertCast(f, type.Array(type.int, 2), type.CAST_ERROR);
        assertCast(f, type.IncompleteArray(type.int), type.CAST_ERROR);
      });
    });

    describe('FunctionUntyped', function() {
      it('should fail to cast function -> anything', function() {
        // Bare functions cannot even be specified in C (only C++). Referencing
        // a function in C is typed as a function pointer.
        var f = type.FunctionUntyped(type.int);
        assertCast(f, f, type.CAST_ERROR);
        assertCast(f, type.void, type.CAST_ERROR);
        assertCast(f, type.int, type.CAST_ERROR);
        assertCast(f, type.Enum('e'), type.CAST_ERROR);
        assertCast(f, type.Pointer(type.int), type.CAST_ERROR);
        assertCast(f, type.Record('s', 4), type.CAST_ERROR);
        assertCast(f, type.Array(type.int, 2), type.CAST_ERROR);
        assertCast(f, type.IncompleteArray(type.int), type.CAST_ERROR);
      });
    });

    describe('Typedef', function() {
      it('should allow cast of pointer -> same pointer w/ typedef', function() {
        var Pc = type.Pointer(type.char);
        var PPc = type.Pointer(Pc);
        var t = type.Typedef('t', Pc);
        var Pt = type.Pointer(t);
        assertCast(Pc, t, type.CAST_OK_EXACT);
        assertCast(t, Pc, type.CAST_OK_EXACT);
        assertCast(PPc, Pt, type.CAST_OK_EXACT);
        assertCast(Pt, PPc, type.CAST_OK_EXACT);
      });

      it('should allow qualifiers to propagate though typedef', function() {
        var Kt = type.Typedef('t', type.char, type.CONST);
        var Kc = type.char.$qualify(type.CONST);
        var VKt = type.Typedef('t2', Kt, type.VOLATILE);
        var KVc = type.char.$qualify(type.CONST | type.VOLATILE);
        assertCast(type.Pointer(Kt), type.Pointer(Kc), type.CAST_OK_EXACT);
        assertCast(type.Pointer(Kc), type.Pointer(Kt), type.CAST_OK_EXACT);
        assertCast(type.Pointer(VKt), type.Pointer(KVc), type.CAST_OK_EXACT);
        assertCast(type.Pointer(KVc), type.Pointer(VKt), type.CAST_OK_EXACT);
      });
    });

    it('should throw when given a type with unknown kind', function() {
      var kind = 1000;
      var t = type.Type(kind);
      assert.throws(function() {
        t.$canCastTo(t);
      }, /kind/);
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
      var fn0 = type.Function(type.void, [type.int]);
      var fns = [fn0];
      assertBestViable(fns, [type.int], 0);
    });

    it('should work if there are 2 overloads w/ an exact match', function() {
      var Pi = type.Pointer(type.int);
      var fn0 = type.Function(type.void, [type.int]);
      var fn1 = type.Function(type.void, [Pi]);
      var fns = [fn0, fn1];
      assertBestViable(fns, [type.int], 0);
      assertBestViable(fns, [Pi], 1);
    });

    it('should prefer an exact match over a promotion', function() {
      var fn0 = type.Function(type.void, [type.int]);
      var fn1 = type.Function(type.void, [type.short]);
      var fns = [fn0, fn1];
      assertBestViable(fns, [type.int], 0);
      assertBestViable(fns, [type.short], 1);
    });

    it('should prefer a int/float promotion over conversion', function() {
      var fn0 = type.Function(type.void, [type.int]);
      var fn1 = type.Function(type.void, [type.double]);
      var fns = [fn0, fn1];
      assertBestViable(fns, [type.char], 0);
      assertBestViable(fns, [type.float], 1);
    });

    it('should choose a promotion if it is available', function() {
      var fn0 = type.Function(type.void, [type.int]);
      var fn1 = type.Function(type.void, [type.Pointer(type.int)]);
      var fns = [fn0, fn1];
      assertBestViable(fns, [type.short], 0);
    });

    it('should prefer an exact match over a conversion', function() {
      var e = type.Enum('e');
      var fn0 = type.Function(type.void, [e]);
      var fn1 = type.Function(type.void, [type.int]);
      var fns = [fn0, fn1];
      assertBestViable(fns, [e], 0);
    });

    it('should choose a conversion if it is available', function() {
      var e = type.Enum('e');
      var fn0 = type.Function(type.void, [type.int]);
      var fns = [fn0];
      assertBestViable(fns, [e], 0);
    });

    it('should work with multiple arguments', function() {
      var i = type.int;
      var c = type.char;
      var Pi = type.Pointer(i);
      var fn0 = type.Function(type.void, [i, c]);
      var fn1 = type.Function(type.void, [i, Pi]);
      var fns = [fn0, fn1];
      assertBestViable(fns, [i, c], 0);
      assertBestViable(fns, [i, Pi], 1);
    });

    it('should ignore functions that aren\'t viable', function() {
      var i = type.int;
      var c = type.char;
      var fn0 = type.Function(type.void, [i, i]);
      var fn1 = type.Function(type.void, [i, i, i, i]);  // Not viable
      var fns = [fn0, fn1];
      assertBestViable(fns, [i, i], 0);
      assertBestViable(fns, [i, c], 0);
    });

    it('should allow multiple promotions/conversions', function() {
      var i = type.int;
      var Pv = type.Pointer(type.void);
      var Pi = type.Pointer(i);
      var c = type.char;
      var fn0 = type.Function(type.void, [Pi, i]);
      var fns = [fn0];
      assertBestViable(fns, [Pv, c], 0);
    });

    it('should allow multiple promotions/conversions', function() {
      var i = type.int;
      var Pv = type.Pointer(type.void);
      var Pi = type.Pointer(i);
      var c = type.char;
      var fn0 = type.Function(type.void, [Pi, i]);
      var fns = [fn0];
      assertBestViable(fns, [Pv, c], 0);
    });

    it('should fail if there are no viable functions', function() {
      var i = type.int;
      var fn0 = type.Function(type.void, [i, i]);
      var fns = [fn0];
      assertBestViable(fns, [], -1);
      assertBestViable(fns, [i], -1);
    });

    it('should fail if no function is best', function() {
      var i = type.int;
      var c = type.char;
      var fn0 = type.Function(type.void, [c, i]);
      var fn1 = type.Function(type.void, [i, c]);
      var fns = [fn0, fn1];
      assertBestViable(fns, [c, c], -1);
    });

    it('should work with variadic functions', function() {
      var i = type.int;
      var fn0 = type.Function(type.void, [i], type.VARIADIC);

      assertBestViable([fn0], [i], 0);
      assertBestViable([fn0], [i, i], 0);
      assertBestViable([fn0], [i, i, i], 0);
    });

    it('should choose any other viable function over variadic', function() {
      var i = type.int;
      var c = type.char;
      var f = type.float;
      var fn0 = type.Function(type.void, [i], type.VARIADIC);
      var fn1 = type.Function(type.void, [i, i]);
      var fn2 = type.Function(type.void, [c, f]);
      var fns = [fn0, fn1, fn2];

      assertBestViable(fns, [i, i], 1);
      assertBestViable(fns, [i, c], 1);
      assertBestViable(fns, [c, f], 2);
      assertBestViable(fns, [i, f], 0);
      assertBestViable(fns, [i], 0);
      assertBestViable(fns, [i, i, i], 0);
    });

    it('should work with functions without a prototype', function() {
      var i = type.int;
      var f = type.float;
      var vp = type.Pointer(type.void);
      var fn = type.FunctionNoProto(type.void);
      var fns = [fn];

      assertBestViable(fns, [], 0);
      assertBestViable(fns, [i], 0);
      assertBestViable(fns, [i, i], 0);
      assertBestViable(fns, [f], 0);
      assertBestViable(fns, [vp], 0);
    });

    it('should work with function pointers', function() {
      var pfn1 = type.Pointer(type.Function(type.int, [type.int]));
      var pfnu = type.Pointer(type.FunctionUntyped());
      var fn2 = type.Function(type.void, [pfn1]);
      var fns = [fn2];

      assertBestViable(fns, [pfn1], 0);
      assertBestViable(fns, [pfnu], 0);
    });
  });

  describe('Qualify', function() {
    it('should combine qualifiers for most types', function() {
      var TV = type.Void;
      var TN = function(cv) { return type.Numeric(type.INT, cv); };
      var TP = function(cv) { return type.Pointer(type.int, cv); };
      var TR = function(cv) { return type.Record('s', 0, type.STRUCT, cv); };
      var TE = function(cv) { return type.Enum('e', cv); };
      var TT = function(cv) { return type.Typedef('t', type.int, cv); };

      // Loop through each type's constructor function.
      [TV, TN, TP, TR, TE, TT].forEach(function(make) {
        var t = make();
        var tc = make(C);
        var tcv = make(CV);
        var tcvr = make(CVR);

        assertTypesEqual(t.$qualify(C), tc);
        assertTypesEqual(t.$qualify(CV), tcv);
        assertTypesEqual(t.$qualify(CVR), tcvr);
        // If already qualified, it is a no-op.
        assertTypesEqual(tc.$qualify(C), tc);
        assertTypesEqual(tcvr.$qualify(CVR), tcvr);
        // Should apply new qualifiers.
        assertTypesEqual(tc.$qualify(V), tcv);
        assertTypesEqual(tc.$qualify(CV), tcv);
      });
    });

    it('should not apply any qualifiers for some types', function() {
      var fp = type.Function(type.void, []);
      var fn = type.FunctionNoProto(type.void);
      var fu = type.FunctionUntyped();
      var ca = type.Array(type.int, 10);
      var ia = type.IncompleteArray(type.int);

      [fp, fn, fu, ca, ia].forEach(function(t) {
        assertTypesEqual(t.$qualify(C), t);
        assertTypesEqual(t.$qualify(VR), t);
        assertTypesEqual(t.$qualify(V), t);
      });
    });
  });

  describe('Unqualified', function() {
    it('should remove qualifiers for all types', function() {
      var TV = type.Void;
      var TN = function(cv) { return type.Numeric(type.INT, cv); };
      var TP = function(cv) { return type.Pointer(type.int, cv); };
      var TR = function(cv) { return type.Record('s', 0, type.STRUCT, cv); };
      var TE = function(cv) { return type.Enum('e', cv); };
      var TT = function(cv) { return type.Typedef('t', type.int, cv); };
      var TFP = function(cv) { return type.Function(type.void, []); };
      var TFN = function(cv) { return type.FunctionNoProto(type.void); };
      var TFU = function(cv) { return type.FunctionUntyped(); };
      var TCA = function(cv) { return type.Array(type.int, 10); };
      var TIA = function(cv) { return type.IncompleteArray(type.int); };

      // Loop through each type's constructor function.
      [TV, TN, TP, TR, TE, TT, TFP, TFN, TFU, TCA, TIA].forEach(function(make) {
        [0, C, V, R, CV, CR, VR, CVR].forEach(function(cv) {
          assertTypesEqual(make(cv).$unqualified(), make());
        });
      });
    });
  });
});
