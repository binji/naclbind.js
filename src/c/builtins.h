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

#ifndef NB_BUILTINS_H_
#define NB_BUILTINS_H_

#define NB_FOREACH_PRIMITIVE(x)     \
  x(voidp, void*);                  \
  x(char, char);                    \
  x(schar, signed char);            \
  x(uchar, unsigned char);          \
  x(short, short);                  \
  x(ushort, unsigned short);        \
  x(int, int);                      \
  x(uint, unsigned int);            \
  x(long, long);                    \
  x(ulong, unsigned long);          \
  x(longlong, long long);           \
  x(ulonglong, unsigned long long); \
  x(float, float);                  \
  x(double, double);

#define NB_FOREACH_ADDSUB(x)                            \
  x(voidp, void*, int);                                 \
  x(int, int, int);                                     \
  x(uint, unsigned int, unsigned int);                  \
  x(longlong, long long, long long);                    \
  x(ulonglong, unsigned long long, unsigned long long); \
  x(float, float, float);                               \
  x(double, double, double);

#define NB_GET(name, t) \
  static inline t nb_get_##name(t* p) { return *p; }
#define NB_SET(name, t) \
  static inline void nb_set_##name(t* p, t x) { *p = x; }
#define NB_LT(name, t) \
  static inline int nb_lt_##name(t a, t b) { return a < b; }
#define NB_LE(name, t) \
  static inline int nb_le_##name(t a, t b) { return a <= b; }
#define NB_GT(name, t) \
  static inline int nb_gt_##name(t a, t b) { return a > b; }
#define NB_GE(name, t) \
  static inline int nb_ge_##name(t a, t b) { return a >= b; }
#define NB_EQ(name, t) \
  static inline int nb_eq_##name(t a, t b) { return a == b; }
#define NB_NE(name, t) \
  static inline int nb_ne_##name(t a, t b) { return a != b; }
#define NB_ADD(name, t1, t2) \
  static inline t1 nb_add_##name(t1 a, t2 b) { return a + b; }
#define NB_SUB(name, t1, t2) \
  static inline t1 nb_sub_##name(t1 a, t2 b) { return a - b; }

NB_FOREACH_PRIMITIVE(NB_GET)
NB_FOREACH_PRIMITIVE(NB_SET)
NB_FOREACH_ADDSUB(NB_ADD)
NB_FOREACH_ADDSUB(NB_SUB)
NB_FOREACH_PRIMITIVE(NB_LT)
NB_FOREACH_PRIMITIVE(NB_LE)
NB_FOREACH_PRIMITIVE(NB_GT)
NB_FOREACH_PRIMITIVE(NB_GE)
NB_FOREACH_PRIMITIVE(NB_EQ)
NB_FOREACH_PRIMITIVE(NB_NE)

/* naclbind-gen:
  -r nb_get_voidp=get
  -r nb_get_char=get
  -r nb_get_schar=get
  -r nb_get_uchar=get
  -r nb_get_short=get
  -r nb_get_ushort=get
  -r nb_get_int=get
  -r nb_get_uint=get
  -r nb_get_long=get
  -r nb_get_ulong=get
  -r nb_get_longlong=get
  -r nb_get_ulonglong=get
  -r nb_get_float=get
  -r nb_get_double=get

  -r nb_set_voidp=set
  -r nb_set_char=set
  -r nb_set_schar=set
  -r nb_set_uchar=set
  -r nb_set_short=set
  -r nb_set_ushort=set
  -r nb_set_int=set
  -r nb_set_uint=set
  -r nb_set_long=set
  -r nb_set_ulong=set
  -r nb_set_longlong=set
  -r nb_set_ulonglong=set
  -r nb_set_float=set
  -r nb_set_double=set

  -r nb_eq_voidp=eq
  -r nb_eq_char=eq
  -r nb_eq_schar=eq
  -r nb_eq_uchar=eq
  -r nb_eq_short=eq
  -r nb_eq_ushort=eq
  -r nb_eq_int=eq
  -r nb_eq_uint=eq
  -r nb_eq_long=eq
  -r nb_eq_ulong=eq
  -r nb_eq_longlong=eq
  -r nb_eq_ulonglong=eq
  -r nb_eq_float=eq
  -r nb_eq_double=eq

  -r nb_ne_voidp=ne
  -r nb_ne_char=ne
  -r nb_ne_schar=ne
  -r nb_ne_uchar=ne
  -r nb_ne_short=ne
  -r nb_ne_ushort=ne
  -r nb_ne_int=ne
  -r nb_ne_uint=ne
  -r nb_ne_long=ne
  -r nb_ne_ulong=ne
  -r nb_ne_longlong=ne
  -r nb_ne_ulonglong=ne
  -r nb_ne_float=ne
  -r nb_ne_double=ne

  -r nb_lt_voidp=lt
  -r nb_lt_char=lt
  -r nb_lt_schar=lt
  -r nb_lt_uchar=lt
  -r nb_lt_short=lt
  -r nb_lt_ushort=lt
  -r nb_lt_int=lt
  -r nb_lt_uint=lt
  -r nb_lt_long=lt
  -r nb_lt_ulong=lt
  -r nb_lt_longlong=lt
  -r nb_lt_ulonglong=lt
  -r nb_lt_float=lt
  -r nb_lt_double=lt

  -r nb_le_voidp=le
  -r nb_le_char=le
  -r nb_le_schar=le
  -r nb_le_uchar=le
  -r nb_le_short=le
  -r nb_le_ushort=le
  -r nb_le_int=le
  -r nb_le_uint=le
  -r nb_le_long=le
  -r nb_le_ulong=le
  -r nb_le_longlong=le
  -r nb_le_ulonglong=le
  -r nb_le_float=le
  -r nb_le_double=le

  -r nb_gt_voidp=gt
  -r nb_gt_char=gt
  -r nb_gt_schar=gt
  -r nb_gt_uchar=gt
  -r nb_gt_short=gt
  -r nb_gt_ushort=gt
  -r nb_gt_int=gt
  -r nb_gt_uint=gt
  -r nb_gt_long=gt
  -r nb_gt_ulong=gt
  -r nb_gt_longlong=gt
  -r nb_gt_ulonglong=gt
  -r nb_gt_float=gt
  -r nb_gt_double=gt

  -r nb_ge_voidp=ge
  -r nb_ge_char=ge
  -r nb_ge_schar=ge
  -r nb_ge_uchar=ge
  -r nb_ge_short=ge
  -r nb_ge_ushort=ge
  -r nb_ge_int=ge
  -r nb_ge_uint=ge
  -r nb_ge_long=ge
  -r nb_ge_ulong=ge
  -r nb_ge_longlong=ge
  -r nb_ge_ulonglong=ge
  -r nb_ge_float=ge
  -r nb_ge_double=ge

  -r nb_add_voidp=add
  -r nb_add_int=add
  -r nb_add_uint=add
  -r nb_add_longlong=add
  -r nb_add_ulonglong=add
  -r nb_add_float=add
  -r nb_add_double=add

  -r nb_sub_voidp=sub
  -r nb_sub_int=sub
  -r nb_sub_uint=sub
  -r nb_sub_longlong=sub
  -r nb_sub_ulonglong=sub
  -r nb_sub_float=sub
  -r nb_sub_double=sub

*/

#undef NB_GET
#undef NB_SET
#undef NB_ADD
#undef NB_SUB
#undef NB_LT
#undef NB_LTE
#undef NB_GT
#undef NB_GTE
#undef NB_EQ
#undef NB_NE
#undef NB_FOREACH_PRIMITIVE
#undef NB_FOREACH_ADDSUB

#endif /* NB_BUILTINS_H_ */
