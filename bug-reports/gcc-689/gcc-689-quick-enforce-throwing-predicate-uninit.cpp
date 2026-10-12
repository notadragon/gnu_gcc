// GCC-689: a potentially-throwing contract predicate checked under
// quick_enforce draws a bogus -Wmaybe-uninitialized warning about a compiler
// temporary.
//
// g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce
//     -O2 -Wall -c
//
// Expected: no diagnostic.  Stock GCC warns "'D.NNNN' may be used
// uninitialized" at the opening brace of each function body.
bool positive (int);

void pre_fn (int x) pre (positive (x)) { }

void assert_fn (int x) { contract_assert (positive (x)); }
