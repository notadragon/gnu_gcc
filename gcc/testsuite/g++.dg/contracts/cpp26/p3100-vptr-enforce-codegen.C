// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_enforce -O0" }

struct S { S () : a (0) {} virtual int v () { return 0; } int a; };
struct T : S { T () : b (0) {} int b; };
int sink (T *p) { return p->b; }

// P3100 Task 4.1: a routed vptr check resolved to a TERMINATING semantic
// (noexcept_enforce) is integrated along the sanitizer's NON-recovering
// (noreturn/abort) code path -- the same path UBSan uses when it will not
// recover -- so the ABORT handler variant is emitted.  This matters because
// vptr is recover-by-default: without forcing the bit off, a terminating
// semantic would land on the recover path and miss the use-after-lifetime
// shapes that only detect on the noreturn path.  finish_options clears
// flag_sanitize_recover for terminating routed checks; ubsan.cc's vptr expander
// then selects the _abort variant.
// { dg-final { scan-assembler "__ubsan_handle_dynamic_type_cache_miss_abort" } }
