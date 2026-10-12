// PR c++/124486 -- [dcl.contract.func]/6: a deleted function shall not have a function-contract-specifier-seq.
// See pr124486-accepted.C for what must still be accepted, including the
// virtual case P3097 allows.
// The pattern says \[^\n\]* rather than .*: DejaGnu matches each
// expectation against the whole compiler output, and .* can run across lines
// and consume another line's diagnostic.  That once made the `= default'
// expectations fail when these cases shared a file.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
struct Del { void f (int x) pre (x > 0) = delete; };
// { dg-error "deleted function \[^\n\]* cannot have a function-contract-specifier" "" { target *-*-* } .-1 }
