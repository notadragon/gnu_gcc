// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=returns-nonnull-attribute -fsanitize-semantic=returns-nonnull-attribute:noexcept_enforce -O0" }

int *g (int *p) __attribute__((returns_nonnull));
int *g (int *p) { return p; }

// P3100 Task 4.1: a routed returns-nonnull-attribute check resolved to
// noexcept_enforce is integrated along the abort code path -> the _abort
// handler variant.
// { dg-final { scan-assembler "__ubsan_handle_nonnull_return_v1_abort" } }
