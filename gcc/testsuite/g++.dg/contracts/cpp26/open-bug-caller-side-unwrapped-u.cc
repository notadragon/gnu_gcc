// Extra translation unit of open-bug-caller-side-unwrapped.C: f, whose
// precondition is ignored on its own side, so it cannot throw; the
// definition of g the caller here is compiled with, which checks nothing
// on the caller's side (no wrapper), so g cannot throw either; and the
// caller.  The linker keeps the g of -k.cc.

int f (int x) pre (x > 0);

int
f (int x)	// callee-side ignore
{
  return x;
}

[[gnu::noinline]] inline int
g (int x)
{
  return f (x);	// no caller-side check
}

int
use_g (int y)
{
  try
    {
      return g (y);
    }
  catch (...)
    {
      return -1;
    }
}
