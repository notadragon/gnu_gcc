// Extra translation unit of refinable-body-ignore-replaceable-catch.C and
// -dtor.C: the inline f with its assertion ignored, and two callers compiled
// with this TU's copy in view.  The linker keeps the copy in -e.cc.

int dtors;
struct D { ~D () { ++dtors; } };

[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);	// ignore
  return x;
}

int use_catch (int y)
{
  try
    {
      return f (y);
    }
  catch (...)
    {
      return -1;
    }
}

int use_dtor (int y)
{
  D d;
  return f (y);
}
