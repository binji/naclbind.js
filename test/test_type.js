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
    describe('Void', function() {
      it('should allow cast of void -> void', function() {
        assert.equal(type.void.canCastTo(type.void), type.CAST_OK);
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
        assert.equal(type.void.canCastTo(e), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(s), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(u), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(f), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(fp), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(p), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(a), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(ia), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(type.bool), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(type.char), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(type.short), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(type.int), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(type.long), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(type.longlong), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(type.uchar), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(type.ushort), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(type.uint), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(type.ulong), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(type.ulonglong), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(type.float), type.CAST_ERROR);
        assert.equal(type.void.canCastTo(type.double), type.CAST_ERROR);
      });
    });

    describe('Numeric', function() {
      it('should allow cast of numeric -> larger numeric', function() {
        assert.equal(type.bool.canCastTo(type.char), type.CAST_OK);
        assert.equal(type.char.canCastTo(type.short), type.CAST_OK);
        assert.equal(type.short.canCastTo(type.int), type.CAST_OK);
        assert.equal(type.int.canCastTo(type.long), type.CAST_OK);
        assert.equal(type.long.canCastTo(type.longlong), type.CAST_OK);
        assert.equal(type.uchar.canCastTo(type.ushort), type.CAST_OK);
        assert.equal(type.ushort.canCastTo(type.uint), type.CAST_OK);
        assert.equal(type.uint.canCastTo(type.ulong), type.CAST_OK);
        assert.equal(type.ulong.canCastTo(type.ulonglong), type.CAST_OK);
        assert.equal(type.float.canCastTo(type.double), type.CAST_OK);
      });

      it('should allow cast of unsigned -> larger signed', function() {
        assert.equal(type.uchar.canCastTo(type.short), type.CAST_OK);
        assert.equal(type.ushort.canCastTo(type.int), type.CAST_OK);
        assert.equal(type.uint.canCastTo(type.long), type.CAST_OK);
        assert.equal(type.ulong.canCastTo(type.longlong), type.CAST_OK);
      });

      it('should warn on cast of unsigned <-> equal-sized signed', function() {
        var signedUnsigned = type.CAST_SIGNED_UNSIGNED;
        assert.equal(type.uchar.canCastTo(type.char), signedUnsigned);
        assert.equal(type.ushort.canCastTo(type.short), signedUnsigned);
        assert.equal(type.uint.canCastTo(type.int), signedUnsigned);
        assert.equal(type.ulong.canCastTo(type.long), signedUnsigned);
        assert.equal(type.ulonglong.canCastTo(type.longlong), signedUnsigned);

        assert.equal(type.char.canCastTo(type.uchar), signedUnsigned);
        assert.equal(type.short.canCastTo(type.ushort), signedUnsigned);
        assert.equal(type.int.canCastTo(type.uint), signedUnsigned);
        assert.equal(type.long.canCastTo(type.ulong), signedUnsigned);
        assert.equal(type.longlong.canCastTo(type.ulonglong), signedUnsigned);
      });

      it('should warn on cast of numeric -> smaller numeric', function() {
        assert.equal(type.char.canCastTo(type.bool), type.CAST_TRUNCATE);
        assert.equal(type.short.canCastTo(type.char), type.CAST_TRUNCATE);
        assert.equal(type.int.canCastTo(type.short), type.CAST_TRUNCATE);
        assert.equal(type.long.canCastTo(type.int), type.CAST_TRUNCATE);
        assert.equal(type.longlong.canCastTo(type.long), type.CAST_TRUNCATE);
        assert.equal(type.ushort.canCastTo(type.uchar), type.CAST_TRUNCATE);
        assert.equal(type.uint.canCastTo(type.ushort), type.CAST_TRUNCATE);
        assert.equal(type.ulong.canCastTo(type.uint), type.CAST_TRUNCATE);
        assert.equal(type.ulonglong.canCastTo(type.ulong), type.CAST_TRUNCATE);
        assert.equal(type.double.canCastTo(type.float), type.CAST_TRUNCATE);
      });

      it('should warn on cast of integral -> pointer-like', function() {
        var c = type.char,
            v = type.void,
            p = type.Pointer(v),
            a = type.Array(c, 2),
            ia = type.IncompleteArray(c);
        [p, a, ia].forEach(function(x) {
          assert.equal(type.char.canCastTo(x), type.CAST_INT_TO_POINTER);
          assert.equal(type.short.canCastTo(x), type.CAST_INT_TO_POINTER);
          assert.equal(type.int.canCastTo(x), type.CAST_INT_TO_POINTER);
          assert.equal(type.long.canCastTo(x), type.CAST_INT_TO_POINTER);
          assert.equal(type.longlong.canCastTo(x), type.CAST_INT_TO_POINTER);
          assert.equal(type.uchar.canCastTo(x), type.CAST_INT_TO_POINTER);
          assert.equal(type.ushort.canCastTo(x), type.CAST_INT_TO_POINTER);
          assert.equal(type.uint.canCastTo(x), type.CAST_INT_TO_POINTER);
          assert.equal(type.ulong.canCastTo(x), type.CAST_INT_TO_POINTER);
          assert.equal(type.ulonglong.canCastTo(x), type.CAST_INT_TO_POINTER);
        });
      });

      it('should fail to cast float -> pointer-like', function() {
        var c = type.char,
            v = type.void,
            p = type.Pointer(v),
            a = type.Array(c, 2),
            ia = type.IncompleteArray(c);
        [p, a, ia].forEach(function(x) {
          assert.equal(type.float.canCastTo(x), type.CAST_ERROR);
          assert.equal(type.double.canCastTo(x), type.CAST_ERROR);
        });
      });

      it('should warn on cast of integral -> enum', function() {
        var e = type.Enum('e');
        assert.equal(type.char.canCastTo(e), type.CAST_INT_TO_ENUM);
        assert.equal(type.short.canCastTo(e), type.CAST_INT_TO_ENUM);
        assert.equal(type.int.canCastTo(e), type.CAST_INT_TO_ENUM);
        assert.equal(type.long.canCastTo(e), type.CAST_INT_TO_ENUM);
        assert.equal(type.longlong.canCastTo(e), type.CAST_INT_TO_ENUM);
        assert.equal(type.uchar.canCastTo(e), type.CAST_INT_TO_ENUM);
        assert.equal(type.ushort.canCastTo(e), type.CAST_INT_TO_ENUM);
        assert.equal(type.uint.canCastTo(e), type.CAST_INT_TO_ENUM);
        assert.equal(type.ulong.canCastTo(e), type.CAST_INT_TO_ENUM);
        assert.equal(type.ulonglong.canCastTo(e), type.CAST_INT_TO_ENUM);
      });

      it('should fail to cast float -> enum', function() {
        var e = type.Enum('e');
        assert.equal(type.float.canCastTo(e), type.CAST_ERROR);
        assert.equal(type.double.canCastTo(e), type.CAST_ERROR);
      });

      it('should fail to cast numeric -> record, void, function', function() {
        var s = type.Record('s', type.Field('f', type.int, 0)),
            u = type.Record('s', type.Field('f', type.int, 0), type.UNION),
            v = type.void,
            f = type.Function(type.void, type.int);
        [s, u, v, f].forEach(function(x) {
          assert.equal(type.char.canCastTo(x), type.CAST_ERROR);
          assert.equal(type.short.canCastTo(x), type.CAST_ERROR);
          assert.equal(type.int.canCastTo(x), type.CAST_ERROR);
          assert.equal(type.long.canCastTo(x), type.CAST_ERROR);
          assert.equal(type.longlong.canCastTo(x), type.CAST_ERROR);
          assert.equal(type.uchar.canCastTo(x), type.CAST_ERROR);
          assert.equal(type.ushort.canCastTo(x), type.CAST_ERROR);
          assert.equal(type.uint.canCastTo(x), type.CAST_ERROR);
          assert.equal(type.ulong.canCastTo(x), type.CAST_ERROR);
          assert.equal(type.ulonglong.canCastTo(x), type.CAST_ERROR);
          assert.equal(type.float.canCastTo(x), type.CAST_ERROR);
          assert.equal(type.double.canCastTo(x), type.CAST_ERROR);
        });
      });
    });

    describe('Pointer', function() {
      it('should allow cast of pointer -> same pointer', function() {
        var v = type.void,
            c = type.char,
            e = type.Enum('e'),
            s = type.Record('s', type.Field('f', type.int, 0)),
            u = type.Record('s', type.Field('f', type.int, 0), type.UNION),
            f = type.Function(type.void, type.int);
        [v, c, e, s, u, f].forEach(function(x) {
          var p = type.Pointer(x),
              a = type.Array(x, 2),
              ia = type.IncompleteArray(x);
          assert.equal(p.canCastTo(p), type.CAST_OK);
          assert.equal(p.canCastTo(a), type.CAST_OK);
          assert.equal(p.canCastTo(ia), type.CAST_OK);
          assert.equal(a.canCastTo(p), type.CAST_OK);
          assert.equal(a.canCastTo(a), type.CAST_OK);
          assert.equal(a.canCastTo(ia), type.CAST_OK);
          assert.equal(ia.canCastTo(p), type.CAST_OK);
          assert.equal(ia.canCastTo(a), type.CAST_OK);
          assert.equal(ia.canCastTo(ia), type.CAST_OK);
        });
      });
    });
  });
});
