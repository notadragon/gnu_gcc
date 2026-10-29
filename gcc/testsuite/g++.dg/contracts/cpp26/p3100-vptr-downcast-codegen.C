// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe" }
// P3100 (UBSan routing): the counterpart of p3100-vptr-downcast-assume-codegen.C
// -- with a checking semantic each of the three casts is instrumented.

struct S { virtual ~S () {} int s = 1; };
struct T : S { int t = 2; };
struct V : virtual S { int v = 3; };

T *ptr (S *p) { return static_cast<T *> (p); }
T &ref (S &r) { return static_cast<T &> (r); }
S *vbase (V *p) { return p; }

// { dg-final { scan-assembler-times "call\[ \t\]+_*__ubsan_handle_dynamic_type_cache_miss" 3 { target i?86-*-* x86_64-*-* } } }
