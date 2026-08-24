// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=vptr -fsanitize-semantic=vptr:assume" }
// P3100 Task 4.1 (UBSan routing, assume): resolving the vptr check to "assume"
// means the check is OFF -- byte identical to a build without -fsanitize=vptr.
// It is realized per function via no_sanitize (cp-gimplify.cc), so NO vptr
// runtime call is emitted.  (assume is exempt from -fcontracts-p4298.)

struct S { S () : a (0) {} virtual int v () { return 0; } int a; };
struct T : S { T () : b (0) {} int b; };

int
access_b (T *p)
{
  return p->b;  // would emit a vptr check if not assumed-away
}

// The vptr runtime handler must NOT be called: the check was assumed away.
// { dg-final { scan-assembler-not "__ubsan_handle_dynamic_type_cache_miss" } }
