// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=address -fsanitize-semantic=address:noexcept_observe -O0" }

int sink (int *p, int i) { return p[i]; }

// P3100 Task 4.1: a routed ASan check whose resolved semantic CONTINUES past
// the violation (noexcept_observe) must be code-generated in RECOVER mode.  The
// routing runtime returns from the report call to resume execution, so the
// recoverable (_noabort) report variant must be emitted -- the non-recoverable
// (noreturn) variant would leave the following access running with call-
// clobbered registers and crash (a stack-buffer-overflow crashed; a heap one
// survived only by register-allocation luck).
// { dg-final { scan-assembler "__asan_report_load\[0-9\]+_noabort" } }
// { dg-final { scan-assembler-not "__asan_report_load\[0-9\]+(?!_noabort)" } }
