// A friend function defined in a class template, with a precondition,
// breaks GCC when the friend is instantiated.  Both cases are valid.
//
// g++ -std=c++26 -fcontracts -fsyntax-only -DCASE=N hidden-friend-template-pre.cpp
//   CASE=1 (predicate names a member through a parameter):
//     internal compiler error: Segmentation fault  (in cp_fold, from
//     instantiate_pending_templates)
//   CASE=2 (predicate names only a parameter):
//     error: could not convert '(((const T)t) > 0)' from
//     'dependent_operator_type' to 'bool'
//   Expected: both compile, and the precondition is checked when the friend
//   is called.
#if CASE == 1
template <class T> struct TS {
  friend void tg(TS s) pre(s.m > 0) {}
  T m;
};
int main() { tg(TS<int>{1}); }
#elif CASE == 2
template <class T> struct S {
  friend void fr2(S, T t) pre(t > 0) {}
};
int main() { fr2(S<int>{}, 1); }
#else
#error define CASE
#endif
