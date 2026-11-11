// Extra translation unit of refinable-body-ignore-replaceable-late.C: the
// same inline f, its assertion enforced.  Listed first, so the linker keeps
// this TU's copy.

inline void h ();

[[gnu::noinline]] inline int f (int x)
{
  h ();
  contract_assert (x > 0);	// enforce
  return x;
}

inline void h () {}

int (*use_fp_e) (int) = f;
