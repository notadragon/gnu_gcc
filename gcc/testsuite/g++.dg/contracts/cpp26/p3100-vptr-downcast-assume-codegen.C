// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=vptr -fsanitize-semantic=vptr:assume" }
// P3100 (UBSan routing, assume): the vptr checks instrumented while parsing
// -- a static_cast downcast, pointer and reference forms
// (ub:expr.static.cast.downcast.wrong.derived.type,
// ub:expr.static.cast.base.class), and a conversion to a virtual base -- are
// off when the check resolves to "assume", as the ones instrumented at
// genericization are: no vptr runtime call is emitted.

struct S { virtual ~S () {} int s = 1; };
struct T : S { int t = 2; };
struct V : virtual S { int v = 3; };

T *ptr (S *p) { return static_cast<T *> (p); }
T &ref (S &r) { return static_cast<T &> (r); }
S *vbase (V *p) { return p; }

// { dg-final { scan-assembler-not "__ubsan_handle_dynamic_type_cache_miss" } }
