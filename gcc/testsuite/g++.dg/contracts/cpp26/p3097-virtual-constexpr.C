// P3097 x constant evaluation: a constexpr virtual function with a contract,
// called through a base reference in a constant expression.  Virtual calls are
// permitted in constant expressions since C++20, so this is usable at compile
// time (the postcondition holds here).
//
// Previously BUG-9: the synthesized contract wrapper for a virtual function was
// not constexpr, so a constexpr virtual function carrying a contract could not
// be constant-evaluated ("call to non-'constexpr' function
// '...contract_wrapper()'").  Now the wrapper inherits the callee's
// constexpr-ness.  See wg21 testing-gap-catalogue.md section 10.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3097" }

struct Base {
  constexpr virtual int f() const post(r: r > 0) { return 1; }
  constexpr virtual ~Base() = default;
};

struct Derived : Base {
  constexpr int f() const override { return 5; }  // post 5 > 0 holds
};

constexpr int test() { Derived d; const Base& b = d; return b.f(); }

constexpr int y = test();          // valid constant
static_assert (y == 5);            // and equals 5
