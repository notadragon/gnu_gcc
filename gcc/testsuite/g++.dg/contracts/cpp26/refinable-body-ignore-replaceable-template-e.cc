// Extra translation unit of refinable-body-ignore-replaceable-template.C:
// the same template, its assertion enforced.  Listed first, so the linker
// keeps this TU's instantiation f<int>.

template <typename T>
[[gnu::noinline]] T f (T x)
{
  contract_assert (x > 0);	// enforce
  return x;
}

int (*use_fp_e) (int) = f<int>;
