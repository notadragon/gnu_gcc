// GCC-35: an ill-formed contract predicate on a DECLARED-ONLY function
// template that is called is never diagnosed.
//
//   g++ -std=c++26 -fcontracts -fsyntax-only <this file>     -> accepted
//   clang++ -std=c++26 -fcontracts -fsyntax-only <this file> -> error
//
// [dcl.contract.func]/9: a function's contract assertions are needed when it
// is odr-used OR defined.  There is no definition here, so the odr-use in
// use() is the only thing that can reach the predicate -- and GCC does not,
// because it substitutes contracts in instantiate_body and nothing else.
//
// GCC-34 fixed the neighbouring case, where a P3097 wrapper or a P3595
// caller-side check needs the contracts of a function that is never defined;
// maybe_instantiate_contracts is called from define_one_contract_wrapper_func.
// With plain callee-side checking there is no wrapper, so nothing calls it and
// the predicate stays dependent.
//
// Severity is limited to the missing diagnostic: with no definition in this
// translation unit GCC emits no check either way, so no contract goes silently
// unchecked at run time.  Contrast the [dcl.contract.func]/7 const-parameter
// rule, which GCC DOES apply to exactly this shape -- see the second function
// below, which GCC rejects correctly.

// -- not diagnosed by GCC; Clang errors on the member access ----------------

template <class T> int f (T t) pre (t.nonexistent ());

int use () { return f (0); }

// -- control: the /7 const-parameter rule IS applied here, by both ----------
//
// Uncomment to confirm GCC diagnoses this one:
//
//   template <class T> int g (T t) post (r: t == r);
//   int use_g () { return g (0); }
