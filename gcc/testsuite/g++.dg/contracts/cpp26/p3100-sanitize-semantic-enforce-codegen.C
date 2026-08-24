// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=address -fsanitize-semantic=address:noexcept_enforce -O0" }

int sink (int *p, int i) { return p[i]; }

// P3100 Task 4.1: a routed ASan check resolved to a TERMINATING semantic
// (noexcept_enforce) keeps the default non-recoverable (noreturn) report call:
// the routing runtime terminates, so there is no continue path to protect and
// the tighter noreturn codegen is correct.  Verify the aborting variant is
// emitted and the recoverable (_noabort) one is not.
// { dg-final { scan-assembler "__asan_report_load\[0-9\]+" } }
// { dg-final { scan-assembler-not "__asan_report_load\[0-9\]+_noabort" } }
