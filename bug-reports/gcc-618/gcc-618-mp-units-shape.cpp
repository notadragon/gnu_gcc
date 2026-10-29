// GCC-618, in the shape it was found in: mpusz/mp-units'
// src/core/include/mp-units/framework/scaling.h, get_one/div_round.  mp-units
// already notes the defect there ("Deferred reason 4: reached from
// `div_round` during constant evaluation") and disables the precondition on
// GCC <= 16; a compiler reporting 17 re-enables it and hits this.  The
// templates and get_zero are not needed to reproduce (see the writeup).
//
//   g++ -std=c++26 -fcontracts -c gcc-618-mp-units-shape.cpp

template<typename T>
constexpr T get_zero(const T& value) { return value - value; }

template<typename T>
constexpr T get_one(const T& value) pre(value != get_zero(value))
{
  return value / value;
}

template<typename T, typename D>
constexpr auto div_round(const T& dividend, const D& divisor)
  pre(divisor > get_zero(divisor))
{
  using quot_type = decltype(dividend / divisor);
  const quot_type quot = dividend / divisor;
  const quot_type step = get_one(divisor);   // binding the result is what triggers it
  return quot + step;
}

constexpr int test() { return div_round(1000, 8); }

static_assert(test() == 126);
