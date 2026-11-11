// Extra translation unit of p4405-premise-multi-tu-inlined.C: h is
// always_inline, inlined into g and its shadow alike, so the precondition
// enforced here is a premise this TU decides, and g's diagnose evaluation is
// sound and assumed.

[[gnu::always_inline]] inline int h (int x) pre (x > 0)	// enforce
{
  return x;
}

int g (int y)
{
  int r = h (y);
  contract_assert (y > 0);	// diagnose: sound, assumed
  return r + (y > 0 ? 1 : 0);
}

