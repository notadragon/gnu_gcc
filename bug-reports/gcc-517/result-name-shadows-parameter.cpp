// A postcondition result name that conflicts with a parameter (or, for a
// lambda, with a capture) is ill-formed ([basic.scope.contract]), whatever
// the evaluation semantic and whether or not the declaration is a definition.
//
//   g++ -std=c++26 -fcontracts -fsyntax-only -fcontract-evaluation-semantic=ignore -DCASE=1
//   g++ -std=c++26 -fcontracts -fsyntax-only -DCASE=2
//   g++ -std=c++26 -fcontracts -fsyntax-only -DCASE=3
//
// Expected: an error for each case.  GCC gives none (case 1 is diagnosed
// only when the semantic is not ignore).
#if CASE == 1
int f (int r) post (r: r > 0) { return r; }             // definition, ignore
#elif CASE == 2
int g (int r) post (r: r > 0);                            // declaration only
#elif CASE == 3
auto l = [x = 1] (int y) post (x: x > 0) { return y; };   // init-capture
#else
#error define CASE
#endif
