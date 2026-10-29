// Extra translation unit of refinable-fork-sem.C: the definitions the
// callers here are compiled with, each assertion evaluated with the semantic
// noted, and the callers.  These copies are not the ones the linker keeps.

#include <contracts>

// Selected dynamically for b_dyn; visible here, so the selection folds.
std::contracts::evaluation_semantic
refinable_fork_sem_sel ()
{
  return std::contracts::evaluation_semantic::ignore;
}

[[gnu::noinline]] inline int
a_enforce (int x)
{
  contract_assert (x > 0);	// enforce
  return x;
}

[[gnu::noinline]] inline int
a_nx_enforce (int x)
{
  contract_assert (x > 0);	// noexcept_enforce
  return x;
}

[[gnu::noinline]] inline int
a_quick (int x)
{
  contract_assert (x > 0);	// quick_enforce
  return x;
}

[[gnu::noinline]] inline int
a_assume (int x)
{
  contract_assert (x > 0);	// assume
  return x;
}

[[gnu::noinline]] inline int
a_vs_observe (int x)
{
  contract_assert (x > 0);	// enforce
  return x;
}

[[gnu::noinline]] inline int
b_ignore (int x)
{
  contract_assert (x > 0);	// ignore
  return x;
}

[[gnu::noinline]] inline int
b_quick (int x)
{
  contract_assert (x > 0);	// quick_enforce
  return x;
}

[[gnu::noinline]] inline int
b_nx_enforce (int x)
{
  contract_assert (x > 0);	// noexcept_enforce
  return x;
}

[[gnu::noinline]] inline int
b_nx_observe (int x)
{
  contract_assert (x > 0);	// noexcept_observe
  return x;
}

[[gnu::noinline]] inline int
b_assume (int x)
{
  contract_assert (x > 0);	// assume
  return x;
}

[[gnu::noinline]] inline int
b_dyn (int x)
{
  contract_assert (x > 0);	// dynamic, ignore
  return x;
}

int use_a_enforce (int y) { return a_enforce (y) > 0; }
int use_a_nx_enforce (int y) { return a_nx_enforce (y) > 0; }
int use_a_quick (int y) { return a_quick (y) > 0; }
int use_a_assume (int y) { return a_assume (y) > 0; }
int use_a_vs_observe (int y) { return a_vs_observe (y) > 0; }

int use_b_ignore (int y) { try { return b_ignore (y); } catch (int) { return -1; } }
int use_b_quick (int y) { try { return b_quick (y); } catch (int) { return -1; } }
int use_b_nx_enforce (int y) { try { return b_nx_enforce (y); } catch (int) { return -1; } }
int use_b_nx_observe (int y) { try { return b_nx_observe (y); } catch (int) { return -1; } }
int use_b_assume (int y) { try { return b_assume (y); } catch (int) { return -1; } }
int use_b_dyn (int y) { try { return b_dyn (y); } catch (int) { return -1; } }
