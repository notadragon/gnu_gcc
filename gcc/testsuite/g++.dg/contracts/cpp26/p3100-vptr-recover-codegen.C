// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe -O0" }

struct S { S () : a (0) {} virtual int v () { return 0; } int a; };
struct T : S { T () : b (0) {} int b; };
int sink (T *p) { return p->b; }

// P3100 Task 4.1: a routed vptr check whose resolved semantic CONTINUES past the
// violation (noexcept_observe) must be code-generated in RECOVER mode -- the
// NON-abort handler variant.  Otherwise the aborting variant's trailing Die()
// (runs after the routed ScopedReport destructor returns for observe) would kill
// the continue.  Verify the recover variant is emitted and the _abort one is not.
// { dg-final { scan-assembler "__ubsan_handle_dynamic_type_cache_miss" } }
// { dg-final { scan-assembler-not "__ubsan_handle_dynamic_type_cache_miss_abort" } }
