// GCC-618 (no upstream PR found): a contract assertion met while deciding
// whether a const local is constant-initialized is evaluated, and a false or
// not-constant predicate recorded.  When the rest of the initializer is not
// constant either, nothing recorded is reported: the local is initialized at
// run time and the predicate is evaluated there.  A by-value parameter hid
// the bug, because copying the unknown argument fails before the precondition
// is reached.
//
// Mirror: clang/test/Contracts/precondition-const-local-init.cpp.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

// The precondition reads the caller's parameter, which is not a constant.
constexpr int
get_one (const int &value) pre (value != 0) // { dg-bogus "not constant" }
{
  return value / value;
}

constexpr int
wrapper (int divisor)
{
  const int step = get_one (divisor);
  return step;
}

// Constant-evaluated, the precondition holds.
static_assert (wrapper (8) == 1);

// The precondition is constant and false, but the body divides by zero, so
// the initializer is not constant either way and the violation belongs to
// run time.
constexpr int
get_one_b (const int &value) pre (value != 0) // { dg-bogus "predicate is false" }
{
  return value / value;
}

int
runtime_only ()
{
  const int step = get_one_b (0);
  return step;
}
