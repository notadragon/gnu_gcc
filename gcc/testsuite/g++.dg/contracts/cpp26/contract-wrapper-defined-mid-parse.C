// A constant evaluation that needs a contract wrapper's body defines it on
// demand, and can do so in the middle of parsing a function, a class, a
// template or a lambda.  The wrapper used to be defined from wherever the
// evaluation happened, with nothing saved, so the enclosing function lost
// its context: its later uses of its own locals were rejected ("use of
// local variable with automatic storage from containing function") and
// compilation then stopped (GCC-199).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

// One class per context, so that each has a wrapper of its own to define.
template <int N>
struct B {
  constexpr virtual int f (int x) const pre (x > 0) { return x; }
};
template <int N>
struct D : B<N> {
  constexpr int f (int x) const override { return x + N; }
};

int
in_function (int a)
{
  int local = a;
  static constexpr D<1> d;
  static constexpr const B<1> &b = d;
  static_assert (b.f (1) == 2);
  local += 3;
  return local;
}

struct in_class {
  static constexpr D<2> d {};
  static constexpr const B<2> &b = d;
  static_assert (b.f (2) == 4);
  int m = 4;
  int h () { return m; }
};

template <class T>
T
in_template (T a)
{
  T local = a;
  static constexpr D<3> d;
  static constexpr const B<3> &b = d;
  static_assert (b.f (3) == 6);
  return local + 1;
}

int
in_lambda (int a)
{
  auto l = [a] {
    int local = a;
    static constexpr D<4> d;
    static constexpr const B<4> &b = d;
    static_assert (b.f (4) == 8);
    return local * 2;
  };
  return l ();
}

int
main ()
{
  if (in_function (1) != 4 || in_class ().h () != 4 || in_template (3) != 4
      || in_lambda (5) != 10)
    __builtin_abort ();
}
