// GCC-618, second form: the precondition is constant and false, but the
// callee's body divides by zero, so the initializer is not a constant
// expression with contracts ignored and `step` is initialized at run time,
// where the violation belongs.  GCC rejects it at compile time.
//
//   g++ -std=c++26 -fcontracts -c gcc-618-runtime-only-violation.cpp
//
//   error: contract predicate is false in constant expression

constexpr int get_one(const int& value) pre(value != 0)
{
  return value / value;
}

int runtime_only()
{
  const int step = get_one(0);
  return step;
}
