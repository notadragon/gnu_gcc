// GCC-36: the [dcl.contract.func]/7 const-parameter rule is reported twice for
// a function template.
//
//   g++ -std=c++26 -fcontracts -fsyntax-only <this file>
//
//   :NN:27: error: value parameter 'v' used in a postcondition must be const
//   :NN:41: error: a value parameter used in a postcondition must be const
//
// One mistake, two diagnostics, from the two mechanisms that both implement
// the rule: check_postcondition_parm_in_redecl (contracts.cc), carrying the
// "odr-used in a postcondition" property from the pattern to the instantiation
// -- an instantiation is a redeclaration of its pattern -- and the walk over
// the substituted predicate in check_postcondition_odr_use_r.
//
// A non-template function gets only the walk, and so reports once.

template <class T> T f (T v) post (r: v == r) { return v; }

int use () { return f (1); }
