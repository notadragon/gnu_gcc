// Extra translation unit of refinable-fork-sem.C, linked first: the
// definitions the linker keeps, each assertion evaluated with the semantic
// noted.

[[gnu::noinline]] inline int
a_enforce (int x)
{
  contract_assert (x > 0);	// ignore
  return x;
}

[[gnu::noinline]] inline int
a_nx_enforce (int x)
{
  contract_assert (x > 0);	// ignore
  return x;
}

[[gnu::noinline]] inline int
a_quick (int x)
{
  contract_assert (x > 0);	// ignore
  return x;
}

[[gnu::noinline]] inline int
a_assume (int x)
{
  contract_assert (x > 0);	// ignore
  return x;
}

[[gnu::noinline]] inline int
a_vs_observe (int x)
{
  contract_assert (x > 0);	// observe
  return x;
}

[[gnu::noinline]] inline int
b_ignore (int x)
{
  contract_assert (x > 0);	// enforce
  return x;
}

[[gnu::noinline]] inline int
b_quick (int x)
{
  contract_assert (x > 0);	// enforce
  return x;
}

[[gnu::noinline]] inline int
b_nx_enforce (int x)
{
  contract_assert (x > 0);	// enforce
  return x;
}

[[gnu::noinline]] inline int
b_nx_observe (int x)
{
  contract_assert (x > 0);	// enforce
  return x;
}

[[gnu::noinline]] inline int
b_assume (int x)
{
  contract_assert (x > 0);	// enforce
  return x;
}

[[gnu::noinline]] inline int
b_dyn (int x)
{
  contract_assert (x > 0);	// enforce
  return x;
}

int (*volatile keep[]) (int) = {
  a_enforce,
  a_nx_enforce,
  a_quick,
  a_assume,
  a_vs_observe,
  b_ignore,
  b_quick,
  b_nx_enforce,
  b_nx_observe,
  b_assume,
  b_dyn,
};
