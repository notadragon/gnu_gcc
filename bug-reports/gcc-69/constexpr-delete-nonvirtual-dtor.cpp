// g++ -std=c++20 -fsyntax-only constexpr-delete-nonvirtual-dtor.cpp
//
// [expr.delete]/3: deleting an object through a pointer to a base class
// whose destructor is not virtual is undefined, and [expr.const] makes an
// operation with undefined behaviour not a core constant expression.  GCC
// 14.4, 15.3, 16.1, 16.2 and trunk accept this; Clang rejects it ("delete of
// object with dynamic type 'D' through pointer to base class type 'B' with
// non-virtual destructor").

struct B { int a; constexpr ~B () {} };
struct D : B { int b; constexpr ~D () {} };

constexpr bool
f ()
{
  B *b = new D;
  delete b;   // undefined: B has no virtual destructor
  return true;
}

static_assert (f ());   // expected: error, not a constant expression
