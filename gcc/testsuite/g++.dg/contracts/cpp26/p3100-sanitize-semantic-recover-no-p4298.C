// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=address -fsanitize-recover=address" }
int main() { return 0; }
// P3100 Task 4.1: with -fcontracts-p3100 routing active, -fsanitize-recover=
// for the routed "address" check (and no explicit -fsanitize-semantic= override)
// derives noexcept_observe, which requires -fcontracts-p4298.  Without that
// flag there is no non-throwing way to honor the requested continue-on-
// violation (terminating would silently drop it), so this is a hard error at
// option-finalization time.
// { dg-error ".-fsanitize-recover=address. routing to the contract-violation handler requires .-fcontracts-p4298." "" { target *-*-* } 0 }
