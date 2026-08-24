// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=kernel-address -fsanitize-semantic=kernel-address:ignore" }
int main() { return 0; }
// "kernel-address" shares SANITIZE_ADDRESS with "address".  The P3100
// Task 1.3 error must name the exact alias the user typed
// ("kernel-address"), not the other name that happens to share the bit
// ("address").  Regression test for a mislabeling bug where the error
// reported a per-bit canonical name instead of the typed one.
// { dg-error ".-fsanitize-semantic=kernel-address:ignore. is not supported" "" { target *-*-* } 0 }
