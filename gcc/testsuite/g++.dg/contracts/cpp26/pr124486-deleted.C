// PR c++/124486 -- [dcl.contract.func]/6: a deleted function shall not have a function-contract-specifier-seq.
// One case per file: combined into one test, DejaGnu intermittently stops
// matching the `= default' diagnostics even though the compiler emits each at
// exactly the expected line and "excess errors" passes.  Not root-caused, and
// not a property of the fix.  See pr124486-accepted.C for what must still be
// accepted, including the virtual case P3097 allows.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
struct Del { void f (int x) pre (x > 0) = delete; };
// { dg-error "deleted function .* cannot have a function-contract-specifier" "" { target *-*-* } .-1 }
