// Extra translation unit of refinable-body-enforce-replaceable.C: the same
// inline f, its assertion ignored, and this TU's copy emitted.

[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);	// ignore
  return x;
}

int (*use_fp) (int) = f;
