// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=shift -fsanitize-semantic=shift-exponent:ignore" }
int main() { return 0; }
// "shift-exponent" is a bit exclusive to the multi-bit "shift" entry
// (SANITIZE_SHIFT_EXPONENT is not the lowest bit of SANITIZE_SHIFT).
// The P3100 allowed-set check must still reject "ignore" for
// it (ignore is never allowed), naming the exact check the user typed.
// Per-bit canonicalization that validates only a multi-bit check's lowest
// bit lets shift-exponent:ignore through silently; this pins that it does
// not.
// { dg-error ".-fsanitize-semantic=shift-exponent:ignore. is not supported" "" { target *-*-* } 0 }
