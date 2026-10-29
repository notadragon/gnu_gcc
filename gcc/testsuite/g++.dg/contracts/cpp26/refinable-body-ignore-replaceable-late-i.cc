// Extra translation unit of refinable-body-ignore-replaceable-late.C: the
// inline f with its assertion ignored.  f calls h, defined only after it,
// so the front end cannot tell that f does not throw; the middle end can.

int dtors;
struct D { ~D () { ++dtors; } };

inline void h ();

[[gnu::noinline]] inline int f (int x)
{
  h ();
  contract_assert (x > 0);	// ignore
  return x;
}

int use_dtor (int y)
{
  D d;
  return f (y);
}

inline void h () {}
