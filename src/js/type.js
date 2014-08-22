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

// Matches LLVM TypeKind names (but not values).
var INVALID = 0,
    UNEXPOSED = 1,
    VOID = 2,
    BOOL = 3,
    CHAR_U = 4,
    UCHAR = 5,
    USHORT = 6,
    UINT = 7,
    ULONG = 8,
    ULONGLONG = 9,
    CHAR_S = 10,
    SCHAR = 11,
    WCHAR = 12,
    SHORT = 13,
    INT = 14,
    LONG = 15,
    LONGLONG = 16,
    FLOAT = 17,
    DOUBLE = 18,
    POINTER = 19,
    RECORD = 20,
    ENUM = 21,
    TYPEDEF = 22,
    FUNCTIONPROTO = 23,
    CONSTANTARRAY = 24,
    INCOMPLETEARRAY = 25,

    PRIMITIVE_SPELLING = {
      2: 'void',
      3: 'bool',
      4: 'char',
      5: 'unsigned char',
      6: 'unsigned short',
      7: 'unsigned int',
      8: 'unsigned long',
      9: 'unsigned long long',
      10: 'char',
      11: 'signed char',
      12: 'wchar_t',
      13: 'short',
      14: 'int',
      15: 'long',
      16: 'long long',
      17: 'float',
      18: 'double',
    },

    PRIMITIVE_RANK = {
      3: 1,  // bool
      4: 2,  // char (unsigned)
      5: 2,  // unsigned char
      6: 3,  // unsigned short
      7: 4,  // unsigned int
      8: 5,  // unsigned long
      9: 6,  // unsigned long long
      10: 2,  // char (signed)
      11: 2,  // signed char
      12: 4,  // wchar_t
      13: 3,  // short
      14: 4,  // int
      15: 5,  // long
      16: 6,  // long long
      17: 7,  // float
      18: 8,  // double
    },

    PRIMITIVE_SIGNED = {
      4: true,  // char (unsigned)
      5: true,  // unsigned char
      6: true,  // unsigned short
      7: true,  // unsigned int
      8: true,  // unsigned long
      9: true,  // unsigned long long
      10: false,  // char (signed)
      11: false,  // signed char
      12: false,  // wchar_t
      13: false,  // short
      14: false,  // int
      15: false,  // long
      16: false,  // long long
    },

    IS_CONST = 1,
    IS_VOLATILE = 2,
    IS_RESTRICT = 4,

    VARIADIC = true,
    NOT_VARIADIC = false,

    STRUCT = false,
    UNION = true,

    CAST_ERROR = 0,
    CAST_OK = 1,
    CAST_TRUNCATE = 2,
    CAST_SIGNED_UNSIGNED = 3,
    CAST_INT_TO_POINTER = 4,
    CAST_POINTER_TO_INT = 5,
    CAST_DISCARD_QUALIFIER = 6,
    CAST_INT_TO_ENUM = 7,
    CAST_DIFFERENT_ENUMS = 8,
    CAST_INCOMPATIBLE_POINTERS = 9,
    CAST_FUNCTION_POINTER_TO_VOID_POINTER = 10,
    CAST_VOID_POINTER_TO_FUNCTION_POINTER = 11,

    SPELLING_PRECEDENCE = {};

SPELLING_PRECEDENCE[POINTER] = 1;
SPELLING_PRECEDENCE[CONSTANTARRAY] = 2;
SPELLING_PRECEDENCE[INCOMPLETEARRAY] = 2;
SPELLING_PRECEDENCE[FUNCTIONPROTO] = 3;


function kindIsInteger(kind) {
  return kind >= 3 && kind <= 16;
}

function kindIsPointerlike(kind) {
  return kind === POINTER || kind === CONSTANTARRAY ||
         kind === INCOMPLETEARRAY;
}

function getMostCanonical(type) {
  while (type.kind === TYPEDEF) {
    type = type.canonical.qualify(type.cv);
  }

  return type;
}

function getPointerlikePointee(type) {
  if (type.kind === POINTER) {
    return type.pointee;
  } else if (type.kind === CONSTANTARRAY) {
    return type.elementType;
  } else if (type.kind === INCOMPLETEARRAY) {
    return type.elementType;
  }
  return null;
}

function isPointerCompatibleWith(from, to) {
  if (!kindIsPointerlike(to.kind)) {
    return false;
  }

  return from.pointee.isCompatibleWith(getPointerlikePointee(to)) &&
         from.cv === to.cv;
}

function canCastPointerTo(from, to) {
  var fp = getPointerlikePointee(from),
      tp;
  if (kindIsPointerlike(to.kind)) {
    tp = getPointerlikePointee(to);
    // Cast to/from void* is always legal.
    if (fp.kind !== VOID && tp.kind !== VOID && !fp.isCompatibleWith(tp)) {
      return CAST_INCOMPATIBLE_POINTERS;
    } else if (fp.kind === VOID && tp.kind === FUNCTIONPROTO) {
      return CAST_VOID_POINTER_TO_FUNCTION_POINTER;
    } else if (fp.kind === FUNCTIONPROTO && tp.kind === VOID) {
      return CAST_FUNCTION_POINTER_TO_VOID_POINTER;
    }

    // If there is a qualifier in |this| that is not set in |that|, it is an
    // error. Note that these are C rules; C++ rules for qualifiers are more
    // restrictive.
    if ((from.cv & ~to.cv) !== 0) {
      return CAST_DISCARD_QUALIFIER;
    }

    return CAST_OK;
  } else if (kindIsInteger(to.kind)) {
    return CAST_POINTER_TO_INT;
  } else {
    return CAST_ERROR;
  }
}

function isLessQualified(q1, q2) {
  return (q2 & ~q1) !== 0 && (q1 & q2) === q1;
}

function isMoreQualified(q1, q2) {
  return q1 !== q2 && isLessQualified(q2, q1);
}


function Type(kind, cv) {
  if (!(this instanceof Type)) { return new Type(kind, cv); }
  this.kind = kind;
  this.cv = cv || 0;
}
Type.prototype.qualify = function(cv) {
  return null;
};
Type.prototype.isCompatibleWith = function(that) {
  return false;
};
Type.prototype.canCastTo = function(that) {
  return CAST_ERROR;
};

function Void(cv) {
  if (!(this instanceof Void)) { return new Void(cv); }
  Type.call(this, VOID, cv);
  this.spelling = getSpelling(this);
}
Void.prototype = Object.create(Type.prototype);
Void.prototype.constructor = Void;
Void.prototype.qualify = function(cv) {
  return Void(this.cv | cv);
};
Void.prototype.isCompatibleWith = function(that) {
  that = getMostCanonical(that);
  return that.kind === VOID;
};
Void.prototype.canCastTo = function(that) {
  that = getMostCanonical(that);
  return that.kind === VOID ? CAST_OK : CAST_ERROR;
};

function Numeric(kind, cv) {
  if (!(this instanceof Numeric)) { return new Numeric(kind, cv); }
  Type.call(this, kind, cv);
  this.spelling = getSpelling(this);
}
Numeric.prototype = Object.create(Type.prototype);
Numeric.prototype.constructor = Numeric;
Numeric.prototype.qualify = function(cv) {
  return Numeric(this.kind, this.cv | cv);
};
Numeric.prototype.isCompatibleWith = function(that) {
  that = getMostCanonical(that);
  return this.kind === that.kind && this.cv === that.cv;
};
Numeric.prototype.canCastTo = function(that) {
  var thisIsInteger,
      thisRank,
      thatRank,
      thisSigned,
      thatSigned;
  that = getMostCanonical(that);
  if (this.constructor !== that.constructor) {
    thisIsInteger = kindIsInteger(this.kind);
    if (thisIsInteger && kindIsPointerlike(that.kind)) {
      return CAST_INT_TO_POINTER;
    } else if (thisIsInteger && that.kind === ENUM) {
      return CAST_INT_TO_ENUM;
    }

    return CAST_ERROR;
  }

  thisRank = PRIMITIVE_RANK[this.kind];
  thatRank = PRIMITIVE_RANK[that.kind];
  thisSigned = PRIMITIVE_SIGNED[this.kind];
  thatSigned = PRIMITIVE_SIGNED[that.kind];
  if (thisRank > thatRank) {
    return CAST_TRUNCATE;
  } else if (thisRank === thatRank && thisSigned !== thatSigned) {
    return CAST_SIGNED_UNSIGNED;
  }

  return CAST_OK;
};

function Pointer(pointee, cv) {
  if (!(this instanceof Pointer)) { return new Pointer(pointee, cv); }
  Type.call(this, POINTER, cv);
  this.pointee = pointee;
  this.spelling = getSpelling(this);
}
Pointer.prototype = Object.create(Type.prototype);
Pointer.prototype.constructor = Pointer;
Pointer.prototype.qualify = function(cv) {
  return Pointer(this.pointee, this.cv | cv);
};
Pointer.prototype.isCompatibleWith = function(that) {
  return isPointerCompatibleWith(this, getMostCanonical(that));
};
Pointer.prototype.canCastTo = function(that) {
  return canCastPointerTo(this, getMostCanonical(that));
};

function Record(tag, fields, isUnion, cv) {
  if (!(this instanceof Record)) {
    return new Record(tag, fields, isUnion, cv);
  }
  Type.call(this, RECORD, cv);
  this.tag = tag;
  this.fields = fields;
  this.isUnion = isUnion || false;
  this.spelling = getSpelling(this);
}
Record.prototype = Object.create(Type.prototype);
Record.prototype.constructor = Record;
Record.prototype.qualify = function(cv) {
  return Record(this.tag, this.fields, this.isUnion, this.cv | cv);
};
Record.prototype.isCompatibleWith = function(that) {
  that = getMostCanonical(that);
  return this.kind === that.kind &&
         this.tag === that.tag &&
         this.cv === that.cv &&
         this.isUnion === that.isUnion;
};
Record.prototype.canCastTo = function(that) {
  return this.isCompatibleWith(getMostCanonical(that)) ? CAST_OK : CAST_ERROR;
};

function Field(name, type, offset) {
  if (!(this instanceof Field)) { return new Field(name, type, offset); }
  this.name = name;
  this.type = type;
  this.offset = offset;
}


function Enum(tag, cv) {
  if (!(this instanceof Enum)) { return new Enum(tag, cv); }
  Type.call(this, ENUM, cv);
  this.tag = tag;
  this.spelling = getSpelling(this);
}
Enum.prototype = Object.create(Type.prototype);
Enum.prototype.constructor = Enum;
Enum.prototype.qualify = function(cv) {
  return Enum(this.tag, this.cv | cv);
};
Enum.prototype.isCompatibleWith = function(that) {
  that = getMostCanonical(that);
  return this.kind === that.kind &&
         this.tag === that.tag &&
         this.cv === that.cv;
};
Enum.prototype.canCastTo = function(that) {
  that = getMostCanonical(that);
  if (this.kind !== that.kind) {
    if (kindIsInteger(that.kind)) {
      return CAST_OK;
    }

    return CAST_ERROR;
  }

  if (this.tag !== that.tag) {
    return CAST_DIFFERENT_ENUMS;
  }

  return CAST_OK;
};

function Typedef(tag, canonical, cv) {
  if (!(this instanceof Typedef)) { return new Typedef(tag, canonical, cv); }
  Type.call(this, TYPEDEF, cv);
  this.tag = tag;
  this.canonical = canonical;
  this.spelling = getSpelling(this);
}
Typedef.prototype = Object.create(Type.prototype);
Typedef.prototype.constructor = Typedef;
Typedef.prototype.qualify = function(cv) {
  return Typedef(this.tag, this.canonical, this.cv | cv);
};
Typedef.prototype.isCompatibleWith = function(that) {
  return getMostCanonical(this).isCompatibleWith(that);
};
Typedef.prototype.canCastTo = function(that) {
  return this.canonical.canCastTo(that);
};

function FunctionProto(resultType, argTypes, variadic) {
  if (!(this instanceof FunctionProto)) {
    return new FunctionProto(resultType, argTypes, variadic);
  }
  Type.call(this, FUNCTIONPROTO, 0);
  this.resultType = resultType;
  this.argTypes = argTypes;
  this.variadic = variadic || false;
  this.spelling = getSpelling(this);
}
FunctionProto.prototype = Object.create(Type.prototype);
FunctionProto.prototype.constructor = FunctionProto;
FunctionProto.prototype.qualify = function(cv) {
  return this;
};
FunctionProto.prototype.isCompatibleWith = function(that) {
  var i;

  that = getMostCanonical(that);

  if (this.kind !== that.kind) {
    return false;
  }

  if (!this.resultType.isCompatibleWith(that.resultType)) {
    return false;
  }

  if (this.argTypes.length !== that.argTypes.length) {
    return false;
  }

  for (i = 0; i < this.argTypes.length; ++i) {
    if (!this.argTypes[i].isCompatibleWith(that.argTypes[i])) {
      return false;
    }
  }

  return true;
};
FunctionProto.prototype.canCastTo = function(that) {
  return CAST_ERROR;
};

function ConstantArray(elementType, arraySize) {
  if (!(this instanceof ConstantArray)) {
    return new ConstantArray(elementType, arraySize);
  }
  Type.call(this, CONSTANTARRAY, 0);
  this.elementType = elementType;
  this.arraySize = arraySize;
  this.spelling = getSpelling(this);
}
ConstantArray.prototype = Object.create(Type.prototype);
ConstantArray.prototype.constructor = ConstantArray;
ConstantArray.prototype.qualify = function(cv) {
  return this;
};
ConstantArray.prototype.isCompatibleWith = function(that) {
  return isPointerCompatibleWith(this, getMostCanonical(that));
};
ConstantArray.prototype.canCastTo = function(that) {
  return canCastPointerTo(this, getMostCanonical(that));
};

function IncompleteArray(elementType) {
  if (!(this instanceof IncompleteArray)) {
    return new IncompleteArray(elementType);
  }
  Type.call(this, INCOMPLETEARRAY, 0);
  this.elementType = elementType;
  this.spelling = getSpelling(this);
}
IncompleteArray.prototype = Object.create(Type.prototype);
IncompleteArray.prototype.constructor = IncompleteArray;
IncompleteArray.prototype.qualify = function(cv) {
  return this;
};
IncompleteArray.prototype.isCompatibleWith = function(that) {
  return isPointerCompatibleWith(this, getMostCanonical(that));
};
IncompleteArray.prototype.canCastTo = function(that) {
  return canCastPointerTo(this, getMostCanonical(that));
};

function getQualifier(cv) {
  var result = '';
  if (cv & IS_CONST) result += 'const ';
  if (cv & IS_VOLATILE) result += 'volatile ';
  if (cv & IS_RESTRICT) result += 'restrict ';
  return result;
}

function describeQualifier(cv) {
  var a;
  if (cv) {
    a = [];
    if (cv & IS_CONST) a.push('const');
    if (cv & IS_VOLATILE) a.push('volatile');
    if (cv & IS_RESTRICT) a.push('restrict');
    return a.join(' ');
  } else {
    return 'none';
  }
}

function getSpelling(type, opt_name, opt_lastKind) {
  var prec,
      lastPrec,
      spelling,
      argsSpelling,
      name;

  spelling = getQualifier(type.cv);
  if (type.kind in PRIMITIVE_SPELLING) {
    spelling += PRIMITIVE_SPELLING[type.kind];
    if (opt_name) {
      spelling += ' ' + opt_name;
    }

    return spelling;
  }

  name = opt_name || '';
  prec = SPELLING_PRECEDENCE[type.kind];
  lastPrec = SPELLING_PRECEDENCE[opt_lastKind];

  if (prec && lastPrec && prec > lastPrec) {
    name = '(' + name + ')';
  }

  if (type.kind === TYPEDEF) {
    spelling += type.tag;
    if (name) {
      spelling += ' ' + name;
    }
  } else if (type.kind === POINTER) {
    name = '*' + spelling + name;
    spelling = getSpelling(type.pointee, name, POINTER);
  } else if (type.kind === ENUM) {
    spelling += 'enum ' + type.tag;
    if (name) {
      spelling += ' ' + name;
    }
  } else if (type.kind === RECORD) {
    if (type.isUnion) {
      spelling += 'union ' + type.tag;
    } else {
      spelling += 'struct ' + type.tag;
    }
    if (name) {
      spelling += ' ' + name;
    }
  } else if (type.kind === CONSTANTARRAY) {
    name += '[' + type.arraySize + ']';
    spelling = getSpelling(type.elementType, name, CONSTANTARRAY);
  } else if (type.kind === INCOMPLETEARRAY) {
    name += '[]';
    spelling = getSpelling(type.elementType, name, INCOMPLETEARRAY);
  } else if (type.kind === FUNCTIONPROTO) {
    name += '(';
    if (type.argTypes.length > 0) {
      argsSpelling = type.argTypes.map(function(a) { return getSpelling(a); });
      if (type.variadic) {
        argsSpelling.push('...');
      }
      name += argsSpelling.join(', ');
    } else {
      name += 'void';
    }
    name += ')';
    spelling = getSpelling(type.resultType, name, FUNCTIONPROTO);
  } else {
    throw new Error('Unknown kind: ' + type.kind);
  }

  return spelling;
}

module.exports = {
  // Types
  void: Void(),
  bool: Numeric(BOOL),
  char: Numeric(CHAR_S),
  uchar: Numeric(UCHAR),
  ushort: Numeric(USHORT),
  uint: Numeric(UINT),
  ulong: Numeric(ULONG),
  ulonglong: Numeric(ULONGLONG),
  schar: Numeric(SCHAR),
  wchar: Numeric(WCHAR),
  short: Numeric(SHORT),
  int: Numeric(INT),
  long: Numeric(LONG),
  longlong: Numeric(LONGLONG),
  float: Numeric(FLOAT),
  double: Numeric(DOUBLE),

  // Type constructors
  Void: Void,
  Numeric: Numeric,
  Pointer: Pointer,
  Record: Record,
  Field: Field,
  Enum: Enum,
  Typedef: Typedef,
  Function: FunctionProto,
  Array: ConstantArray,
  IncompleteArray: IncompleteArray,

  // Qualifiers
  CONST: IS_CONST,
  VOLATILE: IS_VOLATILE,
  RESTRICT: IS_RESTRICT,

  // Type constants
  VARIADIC: VARIADIC,
  NOT_VARIADIC: NOT_VARIADIC,
  STRUCT: STRUCT,
  UNION: UNION,

  // Kinds
  INVALID: INVALID,
  UNEXPOSED: UNEXPOSED,
  VOID: VOID,
  BOOL: BOOL,
  CHAR_U: CHAR_U,
  UCHAR: UCHAR,
  USHORT: USHORT,
  UINT: UINT,
  ULONG: ULONG,
  ULONGLONG: ULONGLONG,
  CHAR_S: CHAR_S,
  SCHAR: SCHAR,
  WCHAR: WCHAR,
  SHORT: SHORT,
  INT: INT,
  LONG: LONG,
  LONGLONG: LONGLONG,
  FLOAT: FLOAT,
  DOUBLE: DOUBLE,
  POINTER: POINTER,
  RECORD: RECORD,
  ENUM: ENUM,
  TYPEDEF: TYPEDEF,
  FUNCTIONPROTO: FUNCTIONPROTO,
  CONSTANTARRAY: CONSTANTARRAY,
  INCOMPLETEARRAY: INCOMPLETEARRAY,

  // Default char to signed
  CHAR: CHAR_S,

  // Cast results
  CAST_ERROR: CAST_ERROR,
  CAST_OK: CAST_OK,
  CAST_TRUNCATE: CAST_TRUNCATE,
  CAST_SIGNED_UNSIGNED: CAST_SIGNED_UNSIGNED,
  CAST_INT_TO_POINTER: CAST_INT_TO_POINTER,
  CAST_POINTER_TO_INT: CAST_POINTER_TO_INT,
  CAST_DISCARD_QUALIFIER: CAST_DISCARD_QUALIFIER,
  CAST_INT_TO_ENUM: CAST_INT_TO_ENUM,
  CAST_DIFFERENT_ENUMS: CAST_DIFFERENT_ENUMS,
  CAST_INCOMPATIBLE_POINTERS: CAST_INCOMPATIBLE_POINTERS,
  CAST_FUNCTION_POINTER_TO_VOID_POINTER: CAST_FUNCTION_POINTER_TO_VOID_POINTER,
  CAST_VOID_POINTER_TO_FUNCTION_POINTER: CAST_VOID_POINTER_TO_FUNCTION_POINTER,

  // Functions
  getSpelling: getSpelling,
  describeQualifier: describeQualifier,
  isLessQualified: isLessQualified,
  isMoreQualified: isMoreQualified,
};
