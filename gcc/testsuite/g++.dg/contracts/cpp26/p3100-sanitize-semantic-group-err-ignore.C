// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=undefined -fsanitize-semantic=undefined:ignore" }
int main() { return 0; }
// "ignore" is never providable by any sanitizer check, so it is a hard
// error even via a GROUP name (unlike observe/quick_enforce, which a
// group silently restricts): a sanitizer check can never be silently
// skipped.  P3100 Task 1.3.
// { dg-error ".-fsanitize-semantic=undefined:ignore. is not supported" "" { target *-*-* } 0 }
