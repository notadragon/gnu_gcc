// Extra translation unit of open-bug-caller-side-unwrapped.C, linked
// first: the definition of g the linker keeps, which checks f's
// precondition on the caller's side (enforce).

int f (int x) pre (x > 0);

[[gnu::noinline]] inline int
g (int x)
{
  return f (x);	// caller-side enforce
}

int (*volatile keep_g) (int) = g;
