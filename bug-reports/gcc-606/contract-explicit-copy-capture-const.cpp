// Within a contract predicate a local entity declared outside it is
// const ([expr.prim.id.unqual]), but a lambda's by-copy capture of it is a
// closure member of the entity's own (non-const) type, so ++v in a mutable
// lambda is OK -- the standard's own example does this with [=].  GCC gives
// an explicitly captured copy the const-qualified type.
//
// g++ -std=c++26 -fcontracts -fsyntax-only contract-explicit-copy-capture-const.cpp
//   -> "increment of read-only variable 'v'" for a1; a2 (implicit [=]) and
//      a3 (init-capture) are accepted.  Expected: all three accepted.
//
void a1() {
  int v = 1;
  contract_assert([v]() mutable { return ++v; }() > 0);     // a1: OK, rejected
}
void a2() {
  int v = 1;
  contract_assert([=]() mutable { return ++v; }() > 0);     // a2: OK, accepted
}
void a3() {
  int v = 1;
  contract_assert([w = v]() mutable { return ++w; }() > 0);  // a3: OK, accepted
}
