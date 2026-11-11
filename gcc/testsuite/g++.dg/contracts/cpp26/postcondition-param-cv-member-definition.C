// A parameter variable takes its top-level cv-qualifiers from the
// definition ([dcl.fct]/5: they are not part of the function type), also for
// a member of a class template defined outside the class, per instance and
// with T = const int (const from a template argument stays), for member
// function templates of a class and of a class template, and an explicit
// specialization of a member; and so a postcondition's const rule applies to
// such a definition's own declaration of the parameter.  Not
// contracts-specific apart from the last case.  (From the EDG fork's
// sema/param_cv_member_definition, EDG-51.)
//
// Mirror: clang/test/Contracts/OpenBugs/param-cv-out-of-class-member-definition.cpp
// in the llvm_llvm-project fork (Clang takes the in-class declaration's
// cv-qualifiers, a stock bug).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

template <class T> struct A {
  void m1(const T x);
  void m2(T x);
  void m3(const T x) { x = 1; }                 // { dg-error "assignment of read-only parameter" } in A<int>
  template <class U> void m4(const T x, U);
  void m5(const T x);
};
template <class T> void A<T>::m1(T x) { x = 1; }  // OK
template <class T> void A<T>::m2(const T x) {
  x = 1;                                         // { dg-error "assignment of read-only parameter" } in A<int>
}
template <class T> template <class U>
void A<T>::m4(T x, U) { x = 1; }                // OK
template <> void A<long>::m5(long x) { x = 1; }  // OK
template <class T> void A<T>::m5(T x) { x = 1; }  // OK
template struct A<int>;
template void A<int>::m4(int, char);
template <> struct A<const int> {
  void n(const int x);
};
void A<const int>::n(int x) { x = 1; }         // OK, not a template member

template <class T> struct B {
  void m1(T x);
  void m2(const T x);
};
template <class T> void B<T>::m1(T x) {
  x = 1;                                         // { dg-error "assignment of read-only parameter" } in B<const int>
}
template <class T> void B<T>::m2(T x) {
  x = 1;                                         // { dg-error "assignment of read-only parameter" } in B<const int>
}
template struct B<const int>;

struct S {
  template <class U> void m(const U x);
  template <class U> void m2(U x);
  template <class U> friend void f(const U x);
  void n(const int x);
};
template <class U> void S::m(U x) { x = 1; }    // OK
template <class U> void S::m2(const U x) {
  x = 1;                                         // { dg-error "assignment of read-only parameter" } in S::m2<int>
}
template <class U> void f(U x) { x = 1; }       // OK
void S::n(int x) { x = 1; }                     // OK
template void S::m(int);
template void S::m2(int);
template void f(int);

template <class T> struct C {
  template <class U> void m(const U x);
};
template <class T> template <class U>
void C<T>::m(U x) { x = 1; }                    // OK
template void C<int>::m(int);

// [dcl.contract.func]: a parameter a postcondition odr-uses must be const in
// every declaration, so also in the definition (EDG-27).
template <class T> struct D {
  void m(const T x) post(x > 0);
  void n(const T x) post(x > 0);
};
template <class T> void D<T>::m(T x) {}         // { dg-error "must be const" } in D<int>
template <class T> void D<T>::n(const T x) {}   // OK
template struct D<int>;
