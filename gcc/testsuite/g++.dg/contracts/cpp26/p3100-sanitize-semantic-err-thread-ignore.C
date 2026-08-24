// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=thread -fsanitize-semantic=thread:ignore" }
int main() { return 0; }
// "ignore" is never in a sanitizer check's allowed set (P3100 Task 1.3): a
// sanitizer check can never be silently skipped, only assumed, enforced, or
// (for a routed check) noexcept_enforce'd / noexcept_observe'd / quick_enforce'd.
// { dg-error ".-fsanitize-semantic=thread:ignore. is not supported" "" { target *-*-* } 0 }
