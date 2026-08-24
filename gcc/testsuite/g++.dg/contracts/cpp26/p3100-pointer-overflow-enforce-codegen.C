// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=pointer-overflow -fsanitize-semantic=pointer-overflow:noexcept_enforce -O0" }

char *add (char *p, unsigned long i) { return p + i; }

// P3100 Task 4.1: a routed pointer-overflow check resolved to noexcept_enforce
// is integrated along the abort code path -> the _abort handler variant.
// { dg-final { scan-assembler "__ubsan_handle_pointer_overflow_abort" } }
