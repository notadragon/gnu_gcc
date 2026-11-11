// GCC-1006 (PR128543): a contract predicate that exits via an exception
// during constant evaluation segfaults the compiler.  The evaluation of the
// predicate leaves its result null and the throw in the jump target, and
// the contract-statement case of cxx_eval_constant_expression passes the
// null result to integer_zerop.  Should be diagnosed (the predicate's
// evaluation exits via an exception, a contract violation, so g () is not
// a constant expression), not crash.
//
//   g++ -std=c++26 -fcontracts -c gcc-1006-throwing-predicate-constexpr-ice.cpp

constexpr bool g () { contract_assert ((throw 1, true)); return true; }
static_assert (g ());
