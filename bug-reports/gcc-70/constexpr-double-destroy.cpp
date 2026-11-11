// g++ -std=c++20 -fsyntax-only constexpr-double-destroy.cpp
//
// The explicit destructor call ends the object's lifetime; the
// delete-expression then runs the destructor on an object outside its
// lifetime ([basic.life], [expr.delete]), which [expr.const] excludes from
// core constant expressions.  GCC 14.4, 15.3, 16.1, 16.2 and trunk accept
// this; Clang rejects it ("destroying object '{*new S#0}' whose lifetime has
// already ended").

struct S { float f = 0; constexpr ~S () {} };

constexpr bool
f ()
{
  S *p = new S;
  p->~S ();
  delete p;   // destroys *p a second time
  return true;
}

static_assert (f ());   // expected: error, not a constant expression
