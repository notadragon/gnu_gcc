// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=return -fsanitize-semantic=return:observe" }
int main() { return 0; }
// The "return" check cannot recover (sanitizer_opts[].can_recover ==
// false), so requesting "observe" for it is outside the check's allowed
// set and is a hard error (P3100 Task 1.3), never silently clamped.
// { dg-error ".-fsanitize-semantic=return:observe. is not supported" "" { target *-*-* } 0 }
