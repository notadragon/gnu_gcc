// PR c++/124486 -- [dcl.contract.func]/6: nor a defaulted destructor.
// See pr124486-accepted.C for what must still be accepted, including the
// virtual case P3097 allows.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
struct DefDtor { ~DefDtor () pre (true) = default; };
// { dg-error "defaulted on its first declaration cannot have a function-contract-specifier" "" { target *-*-* } .-1 }
