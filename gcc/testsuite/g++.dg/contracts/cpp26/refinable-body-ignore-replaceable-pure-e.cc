// Extra translation unit of refinable-body-ignore-replaceable-pure.C: the same
// inline f, its assertion observed.  Listed first, so the linker keeps this
// TU's copy.

[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);	// observe
  return x;
}

int (*use_fp_e) (int) = f;
