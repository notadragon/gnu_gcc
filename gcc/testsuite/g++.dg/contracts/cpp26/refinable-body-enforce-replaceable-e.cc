// Extra translation unit of refinable-body-enforce-replaceable.C: f enforces
// its assertion here.

[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);	// enforce
  return x;
}

int use_f (int y) { return f (y) > 0; }
