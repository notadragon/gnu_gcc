// GCC-954: a requires-expression whose parameter list starts with a
// concept-id, met while parsing a function contract's predicate, aborts the
// compiler during error recovery.
//
//   g++ -std=c++26 -fcontracts -fsyntax-only gcc-954-requires-concept-id-in-predicate-ice.cpp
//   g++ -std=c++26 -fcontracts -fsyntax-only -DCASE_B gcc-954-requires-concept-id-in-predicate-ice.cpp
//
// Stock trunk 17.0.0 20261029 (6aae4093ee7), both cases:
//   error: ...
//   internal compiler error: in cp_parser_function_contract_specifier,
//   at cp/parser.cc:34261
// GCC 16.1.0 / 16.2.0 (release checking): the same abort, reported as
//   "confused by earlier errors, bailing out".
// Expected: the errors alone.

template <class T> concept C = true;

#ifndef CASE_B
// The P4283 spelling of a requires-clause on a contract assertion, which
// stock does not support: "expected '(' before 'requires'", and the
// predicate parse then reads requires(C<int>) as a requires-expression.
void f () pre requires (C<int>);
#else
// An ordinary predicate holding an ill-formed requires-expression.
void f () pre (requires (C<int> x) { x; });
#endif
