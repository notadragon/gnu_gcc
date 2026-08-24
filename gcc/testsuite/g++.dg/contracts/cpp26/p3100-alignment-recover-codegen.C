// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=alignment -fsanitize-semantic=alignment:noexcept_observe -O0" }

int sink (int *p) { return *p; }

// P3100 Task 4.1: a routed alignment check whose resolved semantic CONTINUES
// (noexcept_observe) is integrated along the sanitizer's RECOVER path -- the
// NON-abort handler variant -- so execution can resume after the routed report.
// { dg-final { scan-assembler "__ubsan_handle_type_mismatch_v1" } }
// { dg-final { scan-assembler-not "__ubsan_handle_type_mismatch_v1_abort" } }
