// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=nonnull-attribute -fsanitize-semantic=nonnull-attribute:noexcept_enforce -O0" }

void g (int *p) __attribute__((nonnull (1)));
void call (int *p) { g (p); }

// P3100 Task 4.1: a routed nonnull-attribute check resolved to noexcept_enforce
// is integrated along the abort code path -> the _abort handler variant.
// { dg-final { scan-assembler "__ubsan_handle_nonnull_arg_abort" } }
