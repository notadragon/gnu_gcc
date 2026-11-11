// Extra translation unit of refinable-body-ignore-replaceable-pure.C: the
// inline f with its assertion ignored, and a caller compiled with this TU's
// copy in view.

int g;

[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);	// ignore
  return x;
}

int use_g (int y)
{
  g = 1;
  int r = f (y);
  return r + g;
}
