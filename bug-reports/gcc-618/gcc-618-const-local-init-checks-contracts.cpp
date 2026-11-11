// GCC-618: whether a const local is constant-initialized is decided by
// evaluating its initializer with every contract assertion ignored
// ([expr.const]), but constant evaluation checks the precondition during that
// attempt and rejects a well-formed program.
//
//   g++ -std=c++26 -fcontracts -c gcc-618-const-local-init-checks-contracts.cpp
//
//   error: contract condition is not constant
//   error: non-constant condition for static assertion

constexpr int get_one(const int& value) pre(value != 0)
{
  return value / value;
}

constexpr int wrapper(int divisor)
{
  const int step = get_one(divisor);   // a non-const local compiles clean
  return step;
}

static_assert(wrapper(8) == 1);
