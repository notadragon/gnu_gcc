// A malformed assertion-control label after a function declarator, its
// `>' missing, is a syntax error diagnosed without further trouble: parsing
// recovers, and a later declaration is diagnosed as usual (GCC-662).
// (Without -fcontracts-p3400 the label's <...> is skipped instead, and with
// no `>' that skip runs to the end of the file.)
//
// Mirror: clang/test/Contracts/pre-without-parenthesis.cpp in the
// llvm_llvm-project fork (its `j'; CLANG-638).  Found by the EDG fork's
// parse/errors test.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce" }

void j (int x) pre<1 x;		// { dg-error "expected '>' before 'x'" }
// { dg-error "expected '\\(' before 'x'" "" { target *-*-* } .-1 }
// { dg-error "expected '\\)' before ';'" "" { target *-*-* } .-2 }

int after = undeclared;		// { dg-error "'undeclared' was not declared in this scope" }
