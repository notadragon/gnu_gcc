// PR c++/127196: a redeclaration whose corresponding parameter has a
// DEPENDENT type escapes the postcondition const rule.
//
// [dcl.contract.func]/7 requires the parameter odr-used by the predicate
// "and the corresponding parameter on all declarations of f" to have const
// type.  With concrete types every declaration is checked; with a dependent
// one the declaration that carries the contract was never re-examined once
// the arguments were known, because duplicate_decls merges the declarations
// and the DEFINITION's parameters are the ones that survive.  So the first
// declaration below -- the one with the contract, and the one that is
// wrong -- had simply ceased to exist by instantiation time.
//
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

// The report's own case: contract on the non-const declaration, const on the
// definition.
namespace Reported {
  template <typename T> void f (T a) post (a); // { dg-message "declared .int. here, which is not const" }
  template <typename T> void f (T const a) {}  // { dg-error "value parameter 'a' used in a postcondition must be const" }
  template void f<int> (int);                  // { dg-message "required from here" }
}

// CONTROL, non-dependent: the same shape with concrete types was always
// caught, on the contract's own declaration and at parse time.  This is what
// says the gap was specific to dependence.
namespace NonDependent {
  void f (int a) post (a); // { dg-error "value parameter used in a postcondition must be const" }
  void f (const int a) {}
}

// CONTROL: the contract on the const declaration, the other non-const.  Also
// ill-formed, and always was -- the non-const one is the definition, so it is
// the parameter that survives and gets checked.
// The second diagnostic on each of the next two is pre-existing and shared
// with stock g++ 16.2: the redeclaration check and the walk over the
// substituted predicate both report, on different lines.  Recorded, not
// introduced here.
namespace OtherWayRound {
  template <typename T> void f (T const a) post (a); // { dg-error "value parameter used in a postcondition must be const" }
  template <typename T> void f (T a) {} // { dg-error "value parameter 'a' used in a postcondition must be const" }
  template void f<int> (int);           // { dg-message "required from here" }
}

// CONTROL: both declarations non-const.  Ill-formed with no redeclaration
// question involved.
namespace BothNonConst {
  template <typename T> void f (T a) post (a); // { dg-error "value parameter used in a postcondition must be const" }
  template <typename T> void f (T a) {} // { dg-error "value parameter 'a' used in a postcondition must be const" }
  template void f<int> (int);           // { dg-message "required from here" }
}

// Both const: WELL-FORMED, and the check must not over-reject it.
namespace BothConst {
  template <typename T> void f (T const a) post (a);
  template <typename T> void f (T const a) {}
  template void f<int> (int);
}

// The corner that rules out deciding this structurally at merge time.  The
// declarations disagree about writing `const`, but T is deduced as a const
// type, so BOTH parameters are const after substitution and the program is
// well-formed.  Only substituting the recorded type answers this; comparing
// the two declared types' cv-qualifiers would reject it.
namespace ConstTemplateArgument {
  template <typename T> void f (T a) post (a);
  template <typename T> void f (T const a) {}
  template void f<const int> (const int);
}

// Never instantiated: nothing to substitute, so nothing is diagnosed.
namespace NeverInstantiated {
  template <typename T> void f (T a) post (a);
  template <typename T> void f (T const a) {}
}

// A reference parameter is outside the rule however the declarations differ.
namespace Reference {
  template <typename T> void f (T &a) post (a);
  template <typename T> void f (T &a) {}
  template void f<int> (int &);
}

// Three declarations, with the offending one in the middle, so the record has
// to survive more than a single merge.
namespace ThreeDeclarations {
  template <typename T> void f (T const a) post (a);
  template <typename T> void f (T a);         // { dg-message "declared .int. here, which is not const" }
  template <typename T> void f (T const a) {} // { dg-error "value parameter 'a' used in a postcondition must be const" }
  template void f<int> (int);                 // { dg-message "required from here" }
}
