// GCC-45: a contract specifier written inside a typedef declaration is
// accepted and silently dropped.
//
// A function-contract-specifier-seq is part of a function DECLARATOR
// ([dcl.contract.func]); a typedef-name is not a function declaration, so
// there is no function for the contract to belong to and nothing is ever
// checked at any call.
//
// g++ -std=c++26 -fcontracts -fsyntax-only contract-on-typedef-declaration.cpp
//
// Expected: rejected.  Actual: accepted with no diagnostic at all.

typedef int FTypedef (int) pre (true);

typedef int FTypedefPost (int) post (r : r > 0);

// A member typedef reaches the same place through the class-member path.
struct S {
  typedef int FMember (int) pre (true);
};

// Reached through a template, so it is not specific to the non-dependent
// path either.
template <typename T>
struct U {
  typedef T FDependent (T) pre (true);
};

template struct U<int>;
