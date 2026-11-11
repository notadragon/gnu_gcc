// A member function of a local class inside a lambda in a contract predicate
// names the enclosing function's parameter, which is ill-formed.
//
// Clang: clang/test/Contracts/local-class-in-contract-lambda.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

void f1 (int x) pre ([&] { struct Q { int m () { return x; } }; return true; } ()); // { dg-error "use of parameter from containing function" }
void f2 (int x) pre ([&] { struct Q { int m () { return x; } }; return Q{}.m () > 0; } ()) {} // { dg-error "use of parameter from containing function" }
void f3 (int x) { contract_assert ([&] { struct Q { int m () { return x; } }; return true; } ()); } // { dg-error "use of parameter from containing function" }

void f4 (int x)
{
  contract_assert ([&] { return [&] { struct Q { int m () { return x; } }; return true; } (); } ()); // { dg-error "use of parameter from containing function" }
}
