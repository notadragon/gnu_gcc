// Extra translation unit of p4405-premise-multi-tu-late-summary-1.C and -2.C:
// the same inline f, both of its assertions ignored here.

[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);	// ignore
  contract_assert (x > 0);	// ignore
  return x;
}

// Emit this TU's copy of f.
int (*use_fp) (int) = f;

