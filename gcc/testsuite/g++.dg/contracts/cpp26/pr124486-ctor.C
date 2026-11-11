// PR c++/124486 -- [dcl.contract.func]/6: nor one defaulted on its first declaration.
// See pr124486-accepted.C for what must still be accepted, including the
// virtual case P3097 allows.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
struct DefCtor { DefCtor () pre (true) = default; };
// { dg-error "defaulted on its first declaration cannot have a function-contract-specifier" "" { target *-*-* } .-1 }
