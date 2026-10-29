// Extra translation unit of refinable-body-ignore-replaceable-catch.C and
// -dtor.C: the same inline f, its assertion enforced, so a violation calls
// the (throwing) violation handler.  Listed before -i.cc, so the linker
// keeps this TU's copy.

[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);	// enforce
  return x;
}

int (*use_fp_e) (int) = f;
