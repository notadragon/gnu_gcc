// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=null,vptr -fsanitize-semantic=undefined:quick_enforce -fsanitize-semantic-print" }
int main() { return 0; }
// GROUP name "undefined" with "quick_enforce": mirror stock
// -fsanitize-trap=undefined -- restrict quick_enforce to members that can
// realize it.  A trappable member (null) takes quick_enforce.  vptr is a
// ROUTED check (Task 4.1): quick_enforce for a routed check is realized in the
// routing runtime (a silent terminate), NOT as a compile-time trap, so it is
// available regardless of vptr's can_trap==false capability -- hence vptr also
// takes quick_enforce here.  (Before vptr was routed it stayed at its derived
// observe semantic; the routed allowed set now includes quick_enforce.)  P3100
// Task 1.3 stores quick_enforce PER MEMBER BIT against that bit's own
// (routed-or-capability) allowed set.  Compiles clean.  Only these two checks
// are enabled so the debug seam prints exactly these lines.
// { dg-regexp "null: quick_enforce" }
// { dg-regexp "vptr: quick_enforce" }
