// A contract predicate in a templated entity is instantiated (and thus
// constant-evaluated) only when the entity is potentially constant-evaluated.
// A use in an unevaluated operand does not instantiate the predicate; forcing
// constant evaluation does.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

template<typename T>
constexpr int
is_constexpr () pre (T::error) // { dg-error "not a member of" }
{
  return 0;
}

void
test ()
{
  // Unevaluated operand: is_constexpr<long> is not potentially
  // constant-evaluated, so pre(T::error) is not instantiated and no
  // "no member 'error' in 'long'" error is produced.
  (void) sizeof (is_constexpr<long> ());

  // The braced-init-list forces constant evaluation of is_constexpr<int>, so
  // pre(int::error) is instantiated and is ill-formed.
  (void) sizeof (int { is_constexpr<int> () });
}
