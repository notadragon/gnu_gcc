// A postcondition of a local-class member function may not name an automatic
// variable (or a lambda's by-copy capture) of the enclosing function; only a
// P3098 postcondition capture, which is artificial, is exempt.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

int f ()
{
  int local = 5;
  struct L { static int g () post (r : r == local) { return 0; } }; // { dg-error "use of local variable" }
  return L::g ();
}

int h ()
{
  int x = 5;
  auto l = [x] {
    struct M { static int g () post (r : r == x) { return 0; } }; // { dg-error "use of local variable" }
    return M::g ();
  };
  return l ();
}
