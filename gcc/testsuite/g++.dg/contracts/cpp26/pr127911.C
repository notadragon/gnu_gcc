/* PR c++/127911 -- a VLA captured by reference in a lambda in a function
   template crashed the compiler at instantiation in a C++26 dialect,
   where contracts are on by default, although no contract appears:

     internal compiler error: in set_contract_capture_flag

   When the instantiation of the lambda finds the VLA's capture proxy, it
   clears the proxy's contract-only mark.  The proxy's value expression
   rebuilds the array from the {pointer, bound} capture struct, so it is not
   the plain reference to the closure member the mark used to assume.  This
   is GCC-495's defect reached from valid code.  */
// { dg-do compile { target c++26 } }
// { dg-additional-options "-Wno-vla" }

typedef __INTPTR_TYPE__ intptr_t;

template <typename T> void foo (T n)
{
  intptr_t buf[n];
  [&buf] () { __typeof (buf) baz; } ();
}

void bar () { foo (42); }

template <typename T> int sum (T n)
{
  int buf[n];
  for (T i = 0; i < n; ++i)
    buf[i] = i;
  return [&buf, n] () {
    int s = 0;
    for (T i = 0; i < n; ++i)
      s += buf[i];
    return s;
  } ();
}

int baz () { return sum (4); }

template <typename T> void bar2 (T n)
{
  int buf[n];
  [&] () { __typeof (buf) qux; (void) buf[0]; } ();
}

void quux () { bar2 (3); }
