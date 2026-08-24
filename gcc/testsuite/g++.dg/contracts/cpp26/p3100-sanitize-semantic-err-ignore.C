// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=address -fsanitize-semantic=address:ignore" }
int main() { return 0; }
// "ignore" is never in a sanitizer check's allowed set (P3100 Task 1.3):
// a sanitizer check can never be silently skipped, only assumed,
// enforced, or (capability-permitting) observed/quick_enforce'd.
// { dg-error ".-fsanitize-semantic=address:ignore. is not supported" "" { target *-*-* } 0 }
