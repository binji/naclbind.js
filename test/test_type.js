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
    C = type.CONST,
    CV = type.CONST | type.VOLATILE,
    CR = type.CONST | type.RESTRICT,
    CVR = type.CONST | type.VOLATILE | type.RESTRICT,
    V = type.VOLATILE,
    VR = type.VOLATILE | type.RESTRICT,
    R = type.RESTRICT;

describe('Type', function() {
  describe('is{Less,More}Qualified', function() {
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
               'Expected ' + qual(q1) + ' less qualified than ' + qual(q2));
        assert(type.isMoreQualified(q2, q1),
               'Expected ' + qual(q2) + ' more qualified than ' + qual(q1));
      });

      NotOK.forEach(function(a) {
        var q1 = a[0], q2 = a[1];
        assert(!type.isLessQualified(q1, q2),
               'Expected ' + qual(q1) + ' less qualified than ' + qual(q2));
        assert(!type.isMoreQualified(q2, q1),
               'Expected ' + qual(q2) + ' more qualified than ' + qual(q1));
      });
    });
  });

  describe('Spell', function() {
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
      assert.equal(spell(type.char.qualify(type.CONST)), 'const char');
      assert.equal(spell(type.int.qualify(type.CONST)), 'const int');
      assert.equal(spell(type.int.qualify(cv)), 'const volatile int');
      assert.equal(spell(type.float.qualify(cvr)),
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
      var Kc = type.char.qualify(type.CONST),
          PKc = type.Pointer(Kc),
          PKPc = type.Pointer(type.Pointer(type.char, type.CONST)),
          PKt = type.Pointer(type.Typedef('foo', type.char, type.CONST)),
          PKPKc = type.Pointer(type.Pointer(Kc, type.CONST)),
          PKVi = type.Pointer(type.int.qualify(type.CONST | type.VOLATILE));
      assert.equal(spell(PKc), 'const char *');
      assert.equal(spell(PKPc), 'char *const *');
      assert.equal(spell(PKt), 'const foo *');
      assert.equal(spell(PKPKc), 'const char *const *');
      assert.equal(spell(PKVi), 'const volatile int *');
    });

    it('should correctly spell function types', function() {
      var PKc = type.Pointer(type.char.qualify(type.CONST)),
          f1 = type.Function(type.int, [type.int, type.int]),
          f2 = type.Function(type.int, [PKc], type.VARIADIC),
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

  describe('Cast', function() {
    function spellTypedef(t) {
      if (t.kind === type.TYPEDEF) {
        return 'typedef of ' + spellTypedef(t.canonical);
      }
      return spell(t);
    }

    function assertCastHelper(from, to, expected) {
      var actual = from.canCastTo(to);
      var msg = 'Cast from "' + spellTypedef(from) + '" -> "' +
                                spellTypedef(to) + '": ' +
                'expected: ' + expected + ' actual: ' + actual;
      assert.equal(actual, expected, msg);
    }

    function assertCast(from, to, expected) {
      var tfrom = type.Typedef('from', from),
          ttfrom = type.Typedef('fromfrom', tfrom),
          tto = type.Typedef('to', to),
          ttto = type.Typedef('toto', tto);

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
        assertCast(type.void, type.void, type.CAST_OK);
      });

      it('should fail cast of void -> anything else', function() {
        var e = type.Enum('e'),
            s = type.Record('s', type.Field('f', type.int, 0)),
            u = type.Record('s', type.Field('f', type.int, 0), type.UNION),
            f = type.Function(type.void, type.int),
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
        assertCast(type.bool, type.char, type.CAST_OK);
        assertCast(type.char, type.short, type.CAST_OK);
        assertCast(type.short, type.int, type.CAST_OK);
        assertCast(type.int, type.long, type.CAST_OK);
        assertCast(type.long, type.longlong, type.CAST_OK);
        assertCast(type.uchar, type.ushort, type.CAST_OK);
        assertCast(type.ushort, type.uint, type.CAST_OK);
        assertCast(type.uint, type.ulong, type.CAST_OK);
        assertCast(type.ulong, type.ulonglong, type.CAST_OK);
        assertCast(type.float, type.double, type.CAST_OK);
      });

      it('should allow cast of unsigned -> larger signed', function() {
        assertCast(type.uchar, type.short, type.CAST_OK);
        assertCast(type.ushort, type.int, type.CAST_OK);
        assertCast(type.uint, type.long, type.CAST_OK);
        assertCast(type.ulong, type.longlong, type.CAST_OK);
      });

      it('should warn on cast of unsigned <-> equal-sized signed', function() {
        var signedUnsigned = type.CAST_SIGNED_UNSIGNED;
        assertCast(type.uchar, type.char, signedUnsigned);
        assertCast(type.ushort, type.short, signedUnsigned);
        assertCast(type.uint, type.int, signedUnsigned);
        assertCast(type.ulong, type.long, signedUnsigned);
        assertCast(type.ulonglong, type.longlong, signedUnsigned);

        assertCast(type.char, type.uchar, signedUnsigned);
        assertCast(type.short, type.ushort, signedUnsigned);
        assertCast(type.int, type.uint, signedUnsigned);
        assertCast(type.long, type.ulong, signedUnsigned);
        assertCast(type.longlong, type.ulonglong, signedUnsigned);
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
        var s = type.Record('s', type.Field('f', type.int, 0)),
            u = type.Record('s', type.Field('f', type.int, 0), type.UNION),
            v = type.void,
            f = type.Function(type.void, type.int);
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
          s = type.Record('s', type.Field('f', type.int, 0)),
          u = type.Record('s', type.Field('f', type.int, 0), type.UNION),
          f = type.Function(type.void, type.int);

      it('should allow cast of pointer -> same pointer', function() {
        [v, c, e, s, u, f].forEach(function(x) {
          var p = type.Pointer(x),
              a = type.Array(x, 2),
              ia = type.IncompleteArray(x);
          assertCast(p, p, type.CAST_OK);
          assertCast(p, a, type.CAST_OK);
          assertCast(p, ia, type.CAST_OK);
          assertCast(a, p, type.CAST_OK);
          assertCast(a, a, type.CAST_OK);
          assertCast(a, ia, type.CAST_OK);
          assertCast(ia, p, type.CAST_OK);
          assertCast(ia, a, type.CAST_OK);
          assertCast(ia, ia, type.CAST_OK);
        });
      });

      it('should allow cast of non-void pointer <-> void pointer', function() {
        var pv = type.Pointer(type.void);
        [c, e, s, u].forEach(function(x) {
          var p = type.Pointer(x),
              a = type.Array(x, 2),
              ia = type.IncompleteArray(x);
          assertCast(p, pv, type.CAST_OK);
          assertCast(a, pv, type.CAST_OK);
          assertCast(ia, pv, type.CAST_OK);
          assertCast(pv, p, type.CAST_OK);
          assertCast(pv, a, type.CAST_OK);
          assertCast(pv, ia, type.CAST_OK);
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
              assertCast(q1p, q2p, type.CAST_OK);
            });
          });
        });
      });

      it('should warn on cast of pointer -> less qualified pointer', function() {
        // Also warn if casting to a differently qualified pointer, e.g.
        // const void* => volatile void*
        [v, c, e, s, u, f].forEach(function(x) {
          var p = type.Pointer(x);
          [0, C, CV, CVR, V, VR, R].forEach(function(q1) {
            var q1p = p.qualify(q1);
            [0, C, CV, CVR, V, VR, R].forEach(function(q2) {
              if (type.isLessQualified(q1, q2) || q1 === q2) return;
              var q2p = p.qualify(q2);
              assertCast(q1p, q2p, type.CAST_DISCARD_QUALIFIER);
            });
          });
        });
      });

      it('should allow cast of pointer-like -> qualified pointer', function() {
        // Arrays cannot be qualified, so test unqualified arrays being cast to
        // qualified pointers.
        [v, c, e, s, u, f].forEach(function(x) {
          var p = type.Pointer(x),
              a = type.Array(x, 2),
              ia = type.IncompleteArray(x);
          [C, CV, CVR, V, VR, R].forEach(function(q) {
            var qp = p.qualify(q);
            assertCast(a, qp, type.CAST_OK);
            assertCast(ia, qp, type.CAST_OK);
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
          var q1p = type.Pointer(type.Pointer(type.char).qualify(q1));
          [0, C, V, R, CV, CR, VR, CVR].forEach(function (q2) {
            if (q1 === q2) return;
            var q2p = type.Pointer(type.Pointer(type.char).qualify(q2));
            assertCast(q1p, q2p, type.CAST_INCOMPATIBLE_POINTERS);
          });
        });
      });

      it('should warn on cast of pointer-like -> integral', function() {
        [v, c, e, s, u, f].forEach(function(x) {
          var p = type.Pointer(x),
              a = type.Array(x, 2),
              ia = type.IncompleteArray(x);
          assertCast(p, type.int, type.CAST_POINTER_TO_INT);
          assertCast(a, type.int, type.CAST_POINTER_TO_INT);
          assertCast(ia, type.int, type.CAST_POINTER_TO_INT);
        });
      });

      it('should fail on cast of pointer-like -> float', function() {
        [v, c, e, s, u, f].forEach(function(x) {
          var p = type.Pointer(x),
              a = type.Array(x, 2),
              ia = type.IncompleteArray(x);
          assertCast(p, type.float, type.CAST_ERROR);
          assertCast(p, type.double, type.CAST_ERROR);
        });
      });

      it('should fail on cast of pointer-like -> anything else', function() {
        [v, c, e, s, u, f].forEach(function(x) {
          var p = type.Pointer(x),
              a = type.Array(x, 2),
              ia = type.IncompleteArray(x);
          [v, e, s, u, f].forEach(function(to) {
            assertCast(p, to, type.CAST_ERROR);
            assertCast(a, to, type.CAST_ERROR);
            assertCast(ia, to, type.CAST_ERROR);
          });
        });
      });
    });

    describe('Record', function() {
      var s = type.Record('s', type.Field('f', type.int, 0)),
          s2 = type.Record('s2', type.Field('f', type.int, 0)),
          u = type.Record('s', type.Field('f', type.int, 0), type.UNION);

      it('should allow cast to same record', function() {
        assertCast(s, s, type.CAST_OK);
        assertCast(u, u, type.CAST_OK);
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
        assertCast(e, e, type.CAST_OK);
      });

      it('should warn on cast of enum -> another enum', function() {
        // C allows this, but I'd rather warn.
        assertCast(e, e2, type.CAST_DIFFERENT_ENUMS);
        assertCast(e2, e, type.CAST_DIFFERENT_ENUMS);
      });

      it('should allow cast of enum -> integral', function() {
        assertCast(e, type.bool, type.CAST_OK);
        assertCast(e, type.char, type.CAST_OK);
        assertCast(e, type.short, type.CAST_OK);
        assertCast(e, type.int, type.CAST_OK);
        assertCast(e, type.long, type.CAST_OK);
        assertCast(e, type.longlong, type.CAST_OK);
        assertCast(e, type.uchar, type.CAST_OK);
        assertCast(e, type.ushort, type.CAST_OK);
        assertCast(e, type.uint, type.CAST_OK);
        assertCast(e, type.ulong, type.CAST_OK);
        assertCast(e, type.ulonglong, type.CAST_OK);
      });

      it('should fail on cast of enum -> float', function() {
        assertCast(e, type.float, type.CAST_ERROR);
        assertCast(e, type.double, type.CAST_ERROR);
      });

      it('should fail on cast of enum -> anything else', function() {
        assertCast(e, type.void, type.CAST_ERROR);
        assertCast(e, type.Pointer(type.int), type.CAST_ERROR);
        assertCast(e, type.Record('s', type.Field('f', type.int, 0)), type.CAST_ERROR);
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
        assertCast(f, type.Record('s', type.Field('f', type.int, 0)), type.CAST_ERROR);
        assertCast(f, type.Array(type.int, 2), type.CAST_ERROR);
        assertCast(f, type.IncompleteArray(type.int), type.CAST_ERROR);
      });
    });
  });
});
