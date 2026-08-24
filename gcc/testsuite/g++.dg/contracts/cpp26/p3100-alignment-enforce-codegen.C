// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=alignment -fsanitize-semantic=alignment:noexcept_enforce -O0" }

int sink (int *p) { return *p; }

// P3100 Task 4.1: a routed alignment check resolved to a TERMINATING semantic
// (noexcept_enforce) is integrated along the sanitizer's NON-recovering
// (abort) code path -- finish_options clears flag_sanitize_recover for the
// terminating routed check -- so the ABORT handler variant is emitted.
// { dg-final { scan-assembler "__ubsan_handle_type_mismatch_v1_abort" } }
