// Contracts and SFINAE/concepts: contracts do not participate in
// overload resolution or concept satisfaction
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

#include <type_traits>

template<typename T>
concept Ordered = requires(T a, T b) { a < b; };

// Contracts with concepts
template<Ordered T>
T max_val(const T a, const T b)
  pre (a != b)
  post (r: r == a || r == b)
{
  return a > b ? a : b;
}

// enable_if does not interact with contracts
template<typename T>
auto integral_inc(T x)
  -> std::enable_if_t<std::is_integral_v<T>, T>
  pre (x < 100)
{
  return x + 1;
}

// Contracts on constrained functions
template<typename T>
  requires std::is_arithmetic_v<T>
T square(const T x)
  pre (x >= T{0})
  post (r: r >= T{0})
{
  return x * x;
}

// Contracts do not make a function template non-viable
template<typename T>
int overloaded(T x)
  pre (x > 0)
{
  return 1;
}

template<typename T>
int overloaded(T x, T y)
  pre (x > 0)
  pre (y > 0)
{
  return 2;
}

// Contract on a function that participates in SFINAE context
template<typename T>
auto sfinae_fn(T x) -> decltype(x + 1)
  pre (x >= 0)
{
  return x + 1;
}

// Concepts with contracts — the concept is the constraint, not the contract
template<Ordered T>
const T& min_val(const T& a, const T& b)
  pre (&a != &b)
{
  return a < b ? a : b;
}

void test() {
  max_val(1, 2);
  max_val(1.0, 2.0);

  integral_inc(5);

  overloaded(1);
  overloaded(1, 2);

  sfinae_fn(5);

  square(4);
  square(3.14);

  min_val(1, 2);
}
