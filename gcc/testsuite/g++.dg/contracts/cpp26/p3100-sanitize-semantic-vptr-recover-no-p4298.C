// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=vptr -fsanitize-recover=vptr" }
int main() { return 0; }
// P3100 Task 4.1: -fsanitize-recover=vptr routes the vptr check to the handler
// with a continuing semantic (noexcept_observe), which requires
// -fcontracts-p4298 -- a throwing handler cannot propagate from a routed
// sanitizer check, so without p4298 there is no non-throwing way to honor
// continue-on-violation.  Hard error (no -fcontracts-p4298 here).
// { dg-error ".-fsanitize-recover=vptr. routing to the contract-violation handler requires .-fcontracts-p4298." "" { target *-*-* } 0 }
