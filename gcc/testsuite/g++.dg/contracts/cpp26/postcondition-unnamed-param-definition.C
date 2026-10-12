// A definition that leaves unnamed a parameter that a postcondition on
// another declaration odr-uses must still declare it const
// ([dcl.contract.func]); the error is given on the definition's unnamed
// parameter.  postcondition-unnamed-param-redecl.C has the non-defining
// redeclaration and the function template; this adds the other kinds of
// definition.  Mirrored from the EDG front end, which missed them (EDG-107
// in the edgcpp_compiler fork).
//
// Mirror: tests/tests/contracts/sema/postcondition_unnamed_def_params.sft.cpp
// in the edgcpp_compiler fork;
// clang/test/Contracts/postcondition-unnamed-param-definition.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

void r1 (const int b) post (b > 0);
void r1 (int) {}	// { dg-error "value parameter (.<anonymous>.|.b.) used in a postcondition must be const" }
void r2 (const int b) post (b > 0);
void r2 (const int) {}	// OK

// Only the second parameter is odr-used.
void r3 (const int a, const int b) post (b > 0);
void r3 (int, int) {}	// { dg-error "value parameter (.<anonymous>.|.b.) used in a postcondition must be const" }

// A function-try-block.
void r4 (const int b) post (b > 0);
void r4 (int) try {} catch (...) {}	// { dg-error "value parameter (.<anonymous>.|.b.) used in a postcondition must be const" }

// A function template whose parameter type is not dependent: diagnosed on
// the template definition, with no instantiation.
template <class T> void t2 (const int b) post (b > 0);
template <class T> void t2 (int) {}	// { dg-error "value parameter (.<anonymous>.|.b.) used in a postcondition must be const" }

// Member functions defined outside the class.
struct S
{
  void m (const int b) post (b > 0);
  static void s (const int b) post (b > 0);
};
void S::m (int) {}	// { dg-error "value parameter (.<anonymous>.|.b.) used in a postcondition must be const" }
void S::s (const int) {}	// OK

// A postcondition with a result name.
int r5 (const int b) post (r : r > b);
int r5 (int) { return 1; }	// { dg-error "value parameter (.<anonymous>.|.b.) used in a postcondition must be const" }

// An instance whose parameter type is const.
template <class T> void t3 (const T b) post (b > 0);
template <class T> void t3 (T) {}	// OK
template void t3<const int> (int);
