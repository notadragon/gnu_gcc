// gcc-650-postcondition-discarded-member-access.cpp              -*-C++-*-
//
// GCC-650: a by-value parameter named only as the object expression of a
// discarded member access in a postcondition is not odr-used
// ([basic.def.odr]: it is a potential result of a discarded-value
// expression to which no lvalue-to-rvalue conversion is applied), so it
// need not be const.  The plain discarded parameter (line C) is accepted
// since PR126897's fix on the fork; stock rejects that too.
//
// Expected: accepted.  Actual: "a value parameter used in a postcondition
// must be const" on lines A and B.
//
// g++ -std=c++26 -fcontracts -fsyntax-only gcc-650-postcondition-discarded-member-access.cpp

struct S { int m; };
void mem (S s) post ((s.m, true)) {}            // A
void mem_void (S s) post (((void) s.m, true)) {} // B
void whole (S s) post ((s, true)) {}            // C
