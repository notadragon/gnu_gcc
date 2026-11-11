// -fcontracts-p4299 in C++: a macro named _Pre is expanded as a macro, a _Pre
// produced by a macro's expansion is read as pre, pre and post stay ordinary
// identifiers, and _ContractAssert cannot name a variable.  (From the EDG
// fork's p4299/keyword and spellings_option.)  That _Pre and _Post cannot
// name a variable either is p4299-pre-post-keywords.C.
// Constant evaluation shows the assertions are attached.
//
// Mirror: clang/test/Contracts/p4299-in-cxx-macros.cpp in the
// llvm_llvm-project fork, which also checks _Pre and _Post.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4299 -fcontract-evaluation-semantic=enforce" }

int _ContractAssert = 1;	// { dg-error "expected unqualified-id" }

#define _Pre post
constexpr int f (int x) _Pre (r : r > 0) { return x; }
#undef _Pre
#define PRECONDITION(c) _Pre (c)
constexpr int h (int x) PRECONDITION (x > 0) { return x; }

// The diagnostics point into the macro definitions.
constexpr int vf = f (-1);	// a postcondition
// { dg-error "contract predicate is false" "" { target *-*-* } 15 }
constexpr int vh = h (-1);	// a precondition
// { dg-error "contract predicate is false" "" { target *-*-* } 18 }

int pre = 1, post = 2;		// OK, ordinary identifiers
int g () { return pre + post; }
