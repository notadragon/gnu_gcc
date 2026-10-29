/* P4405 premises decided by this TU (T1-GCC) in C: the semantic of a C99
   inline callee's precondition, the callee always_inline or noinline.  C's
   quick_enforce traps directly, so like enforce and assume it leaves a
   premise; C's noexcept entry point may return as far as the optimizers
   know, so noexcept_enforce leaves none.  Pinned to -O2.  */
/* Mirror: clang/test/Contracts/C/c-p4405-premise-semantic.c  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -fcontracts-p4298 -fcontracts-allow-assume -O2 -Wno-attributes -fdump-tree-contract_shadow" } */
/* { dg-additional-options "-fcontract-configuration-file=${srcdir}/gcc.dg/contracts/c-p4405-premise-semantic.json" } */
/* { dg-skip-if "pinned to -O2" { *-*-* } { "-O0" "-O1" "-O3" "-Os" "-Og" "-flto" } { "" } } */

int sink (int);

__attribute__ ((always_inline)) inline void h_enforce_always (int x) _Pre (x > 0) { sink (x); }

int
d_enforce_always (int x)
{
  h_enforce_always (x);
  _ContractAssert (x > 0);	/* sound */
  return 0;
}

int
i_enforce_always (int x, int y)
{
  h_enforce_always (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

__attribute__ ((noinline)) inline void h_enforce_never (int x) _Pre (x > 0) { sink (x); }

int
d_enforce_never (int x)
{
  h_enforce_never (x);
  _ContractAssert (x > 0);	/* { dg-warning "contract assertion cannot be proven" } */
  return 0;
}

int
i_enforce_never (int x, int y)
{
  h_enforce_never (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* conditional */
  return 0;
}

__attribute__ ((always_inline)) inline void h_quick_enforce_always (int x) _Pre (x > 0) { sink (x); }

int
d_quick_enforce_always (int x)
{
  h_quick_enforce_always (x);
  _ContractAssert (x > 0);	/* sound */
  return 0;
}

int
i_quick_enforce_always (int x, int y)
{
  h_quick_enforce_always (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

__attribute__ ((noinline)) inline void h_quick_enforce_never (int x) _Pre (x > 0) { sink (x); }

int
d_quick_enforce_never (int x)
{
  h_quick_enforce_never (x);
  _ContractAssert (x > 0);	/* { dg-warning "contract assertion cannot be proven" } */
  return 0;
}

int
i_quick_enforce_never (int x, int y)
{
  h_quick_enforce_never (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* conditional */
  return 0;
}

__attribute__ ((always_inline)) inline void h_assume_always (int x) _Pre (x > 0) { sink (x); }

int
d_assume_always (int x)
{
  h_assume_always (x);
  _ContractAssert (x > 0);	/* sound */
  return 0;
}

int
i_assume_always (int x, int y)
{
  h_assume_always (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

__attribute__ ((noinline)) inline void h_assume_never (int x) _Pre (x > 0) { sink (x); }

int
d_assume_never (int x)
{
  h_assume_never (x);
  _ContractAssert (x > 0);	/* { dg-warning "contract assertion cannot be proven" } */
  return 0;
}

int
i_assume_never (int x, int y)
{
  h_assume_never (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* conditional */
  return 0;
}

__attribute__ ((always_inline)) inline void h_observe_always (int x) _Pre (x > 0) { sink (x); }

int
d_observe_always (int x)
{
  h_observe_always (x);
  _ContractAssert (x > 0);	/* { dg-warning "contract assertion cannot be proven" } */
  return 0;
}

int
i_observe_always (int x, int y)
{
  h_observe_always (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

__attribute__ ((noinline)) inline void h_observe_never (int x) _Pre (x > 0) { sink (x); }

int
d_observe_never (int x)
{
  h_observe_never (x);
  _ContractAssert (x > 0);	/* { dg-warning "contract assertion cannot be proven" } */
  return 0;
}

int
i_observe_never (int x, int y)
{
  h_observe_never (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

__attribute__ ((always_inline)) inline void h_noexcept_enforce_always (int x) _Pre (x > 0) { sink (x); }

int
d_noexcept_enforce_always (int x)
{
  h_noexcept_enforce_always (x);
  _ContractAssert (x > 0);	/* { dg-warning "contract assertion cannot be proven" } */
  return 0;
}

int
i_noexcept_enforce_always (int x, int y)
{
  h_noexcept_enforce_always (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

__attribute__ ((noinline)) inline void h_noexcept_enforce_never (int x) _Pre (x > 0) { sink (x); }

int
d_noexcept_enforce_never (int x)
{
  h_noexcept_enforce_never (x);
  _ContractAssert (x > 0);	/* { dg-warning "contract assertion cannot be proven" } */
  return 0;
}

int
i_noexcept_enforce_never (int x, int y)
{
  h_noexcept_enforce_never (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

__attribute__ ((always_inline)) inline void h_ignore_always (int x) _Pre (x > 0) { sink (x); }

int
d_ignore_always (int x)
{
  h_ignore_always (x);
  _ContractAssert (x > 0);	/* { dg-warning "contract assertion cannot be proven" } */
  return 0;
}

int
i_ignore_always (int x, int y)
{
  h_ignore_always (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

__attribute__ ((noinline)) inline void h_ignore_never (int x) _Pre (x > 0) { sink (x); }

int
d_ignore_never (int x)
{
  h_ignore_never (x);
  _ContractAssert (x > 0);	/* { dg-warning "contract assertion cannot be proven" } */
  return 0;
}

int
i_ignore_never (int x, int y)
{
  h_ignore_never (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

/* { dg-final { scan-tree-dump {\(line 20\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 29\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 39\): unproven} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 48\): conditional} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 58\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 67\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 77\): unproven} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 86\): conditional} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 96\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 105\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 115\): unproven} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 124\): conditional} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 134\): unproven} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 143\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 153\): unproven} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 162\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 172\): unproven} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 181\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 191\): unproven} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 200\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 210\): unproven} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 219\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 229\): unproven} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 238\): sound} "contract_shadow" } } */
