// E4: malformed requires-clause syntax on a contract of a templated function
// is diagnosed (the non-templated-function error is covered by p4283-errors.C;
// this exercises a syntactic error that gets past that gate -- a requires-clause
// with no following parenthesized contract condition).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4283" }

template<class T>
int a (T x) pre requires (sizeof (T) > 0) ;
// { dg-error "expected '\\('" "" { target *-*-* } .-1 }
// { dg-error "expected primary-expression" "" { target *-*-* } .-2 }
// { dg-error "expected '\\)'" "" { target *-*-* } .-3 }
