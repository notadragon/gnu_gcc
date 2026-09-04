// A postcondition capture must compile with -fno-exceptions.
//
// It does not.  GCC reports
//   error: exception handling disabled, use '-fexceptions' to enable
// at the closing brace, for EVERY capture shape: a scalar, a class with a
// trivial destructor, a class with a non-trivial destructor, a class with a
// non-trivial copy constructor, several captures at once, and a capture on a
// virtual function.  A postcondition with no capture is unaffected, and so is
// an ordinary local of non-trivial destructor type.
//
// Cause: contracts.cc lowers capture initialization inside a try/catch, so
// that a throwing capture initializer becomes a post_capture violation
// (P3098's phase model).  Both capture sites -- the ordinary one and the
// virtual capture struct -- call begin_handler unconditionally, and
// doing_eh() in cp/except.cc then errors out when flag_exceptions is off.
// The third site in that file, the catch around predicate evaluation, is
// guarded correctly: `check_might_throw` is already false with -fno-exceptions,
// which is why a contract whose predicate calls a potentially-throwing
// function still compiles.
//
// With no exceptions there is nothing to catch, so the try/catch should
// simply not be built.  Clang compiles every shape below with
// -fno-exceptions and produces the same observable behaviour as with them.
//
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fno-exceptions -fcontracts-p3850" }
// { dg-excess-errors "postcondition captures do not compile with -fno-exceptions" { xfail *-*-* } }

struct TrivialDtor
{
  int v;
};

struct NonTrivialDtor
{
  int v;
  ~NonTrivialDtor () {}
};

struct NonTrivialCopy
{
  int v;
  NonTrivialCopy (int v) : v (v) {}
  NonTrivialCopy (const NonTrivialCopy &o) : v (o.v) {}
};

int
scalar_capture (const int x) post[old = x] (old == x)
{
  return x;
}

int
trivial_dtor_capture (const int x) post[old = TrivialDtor{ x }] (old.v == x)
{
  return x;
}

int
non_trivial_dtor_capture (const int x)
    post[old = NonTrivialDtor{ x }] (old.v == x)
{
  return x;
}

int
non_trivial_copy_capture (const int x)
    post[old = NonTrivialCopy (x)] (old.v == x)
{
  return x;
}

int
several_captures (const int x) post[a = x, b = x + 1] (a + 1 == b)
{
  return x;
}

struct Virtual
{
  virtual int f (const int x) post[old = x] (old == x) { return x; }
  virtual ~Virtual () = default;
};
