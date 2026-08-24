// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=kernel-address -fsanitize-semantic=kernel-address:quick_enforce" }
int main() { return 0; }
// "kernel-address" is an INDIVIDUAL check whose flag spans two bits
// (SANITIZE_ADDRESS + SANITIZE_KERNEL_ADDRESS) with distinct per-bit
// canonical owners.  It must still be treated as an individual check
// (only "undefined"/"all" are silently-restricting meta-groups), so a
// request for a semantic it can't support (quick_enforce; ASan family
// can_trap==false) is a hard error, not a silent skip.  Regression guard
// for a group/individual misclassification of multi-bit aliases.
// { dg-error ".-fsanitize-semantic=kernel-address:quick_enforce. is not supported" "" { target *-*-* } 0 }
