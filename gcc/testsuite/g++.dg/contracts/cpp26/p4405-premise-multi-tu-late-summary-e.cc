// Extra translation unit of p4405-premise-multi-tu-late-summary-1.C and -2.C:
// f's diagnose evaluation is proven soundly from its quick_enforce check and
// assumed in f, but f is inline, a refinable body, so use_f does not use its
// result's range.

[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);	// quick_enforce
  contract_assert (x > 0);	// diagnose: sound, assumed in f
  return x;
}

int use_f (int y) { return f (y) > 0; }

