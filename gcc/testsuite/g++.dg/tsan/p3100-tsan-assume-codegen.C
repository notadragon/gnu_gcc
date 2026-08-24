// P3100: thread:assume must not instrument -- byte identical to a build without
// -fsanitize=thread for this function -- realized per function via no_sanitize
// (cp-gimplify.cc).  So no __tsan_ runtime calls are emitted even though
// tsan_init adds -fsanitize=thread.

// { dg-do compile }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fsanitize-semantic=thread:assume" }

int g;
void f (int *p) { *p = g; g = *p; }
int main () { return 0; }

// thread:assume suppresses per-function instrumentation, so no memory-access or
// function-entry instrumentation calls are emitted.  (The module ctor's
// __tsan_init is always present under -fsanitize=thread and is not matched
// here.)
// { dg-final { scan-assembler-not "__tsan_func_entry" } }
// { dg-final { scan-assembler-not "__tsan_read" } }
// { dg-final { scan-assembler-not "__tsan_write" } }
