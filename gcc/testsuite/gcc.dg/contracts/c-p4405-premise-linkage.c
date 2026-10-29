/* P4405 premises decided by this TU (T1-GCC) in C: the callee's linkage and
   inlining.  A callee-side enforce precondition of a static, external,
   static inline, C99 inline (an inline definition: the external definition
   the program calls is another TU's) or weak callee, always_inline, noinline
   or left to the inliner.  As in C++ (g++.dg's p4405-premise-linkage.C), the
   shadow may rely on what it inlined from a replaceable callee only where
   the real function inlined it too, and a call it makes out of line to one
   holding such an evaluation makes it conditional.  Pinned to -O2.  */
/* Mirror: clang/test/Contracts/C/c-p4405-premise-linkage.c  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -O2 -Wno-attributes -fdump-tree-contract_shadow" } */
/* { dg-additional-options "-fcontract-configuration-file=${srcdir}/gcc.dg/contracts/c-p4405-premise-linkage.json" } */
/* { dg-skip-if "pinned to -O2" { *-*-* } { "-O0" "-O1" "-O3" "-Os" "-Og" "-flto" } { "" } } */

int sink (int);

/* static, always */
__attribute__ ((always_inline)) static inline void h_static_always (int x) _Pre (x > 0) { sink (x); }

int
d_static_always (int x)
{
  h_static_always (x);
  _ContractAssert (x > 0);	/* sound */
  return 0;
}

int
i_static_always (int x, int y)
{
  h_static_always (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

/* static, never */
__attribute__ ((noinline)) static void h_static_never (int x) _Pre (x > 0) { sink (x); }

int
d_static_never (int x)
{
  h_static_never (x);
  _ContractAssert (x > 0);	/* { dg-warning "contract assertion cannot be proven" } */
  return 0;
}

int
i_static_never (int x, int y)
{
  h_static_never (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

/* static, auto */
static void h_static_auto (int x) _Pre (x > 0) { sink (x); }

int
d_static_auto (int x)
{
  h_static_auto (x);
  _ContractAssert (x > 0);	/* sound */
  return 0;
}

int
i_static_auto (int x, int y)
{
  h_static_auto (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

/* extern, always */
__attribute__ ((always_inline)) void h_extern_always (int x) _Pre (x > 0) { sink (x); }

int
d_extern_always (int x)
{
  h_extern_always (x);
  _ContractAssert (x > 0);	/* sound */
  return 0;
}

int
i_extern_always (int x, int y)
{
  h_extern_always (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

/* extern, never */
__attribute__ ((noinline)) void h_extern_never (int x) _Pre (x > 0) { sink (x); }

int
d_extern_never (int x)
{
  h_extern_never (x);
  _ContractAssert (x > 0);	/* { dg-warning "contract assertion cannot be proven" } */
  return 0;
}

int
i_extern_never (int x, int y)
{
  h_extern_never (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

/* extern, auto */
void h_extern_auto (int x) _Pre (x > 0) { sink (x); }

int
d_extern_auto (int x)
{
  h_extern_auto (x);
  _ContractAssert (x > 0);	/* sound */
  return 0;
}

int
i_extern_auto (int x, int y)
{
  h_extern_auto (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

/* static_inline, always */
__attribute__ ((always_inline)) static inline void h_static_inline_always (int x) _Pre (x > 0) { sink (x); }

int
d_static_inline_always (int x)
{
  h_static_inline_always (x);
  _ContractAssert (x > 0);	/* sound */
  return 0;
}

int
i_static_inline_always (int x, int y)
{
  h_static_inline_always (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

/* static_inline, never */
__attribute__ ((noinline)) static inline void h_static_inline_never (int x) _Pre (x > 0) { sink (x); }

int
d_static_inline_never (int x)
{
  h_static_inline_never (x);
  _ContractAssert (x > 0);	/* { dg-warning "contract assertion cannot be proven" } */
  return 0;
}

int
i_static_inline_never (int x, int y)
{
  h_static_inline_never (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

/* static_inline, auto */
static inline void h_static_inline_auto (int x) _Pre (x > 0) { sink (x); }

int
d_static_inline_auto (int x)
{
  h_static_inline_auto (x);
  _ContractAssert (x > 0);	/* sound */
  return 0;
}

int
i_static_inline_auto (int x, int y)
{
  h_static_inline_auto (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

/* c99_inline, always */
__attribute__ ((always_inline)) inline void h_c99_inline_always (int x) _Pre (x > 0) { sink (x); }

int
d_c99_inline_always (int x)
{
  h_c99_inline_always (x);
  _ContractAssert (x > 0);	/* sound */
  return 0;
}

int
i_c99_inline_always (int x, int y)
{
  h_c99_inline_always (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

/* c99_inline, never */
__attribute__ ((noinline)) inline void h_c99_inline_never (int x) _Pre (x > 0) { sink (x); }

int
d_c99_inline_never (int x)
{
  h_c99_inline_never (x);
  _ContractAssert (x > 0);	/* { dg-warning "contract assertion cannot be proven" } */
  return 0;
}

int
i_c99_inline_never (int x, int y)
{
  h_c99_inline_never (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* conditional */
  return 0;
}

/* c99_inline, auto */
inline void h_c99_inline_auto (int x) _Pre (x > 0) { sink (x); }

int
d_c99_inline_auto (int x)
{
  h_c99_inline_auto (x);
  _ContractAssert (x > 0);	/* sound */
  return 0;
}

int
i_c99_inline_auto (int x, int y)
{
  h_c99_inline_auto (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* sound */
  return 0;
}

/* weak, never */
__attribute__ ((noinline)) __attribute__ ((weak)) void h_weak_never (int x) _Pre (x > 0) { sink (x); }

int
d_weak_never (int x)
{
  h_weak_never (x);
  _ContractAssert (x > 0);	/* { dg-warning "contract assertion cannot be proven" } */
  return 0;
}

int
i_weak_never (int x, int y)
{
  h_weak_never (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* conditional */
  return 0;
}

/* weak, auto */
__attribute__ ((weak)) void h_weak_auto (int x) _Pre (x > 0) { sink (x); }

int
d_weak_auto (int x)
{
  h_weak_auto (x);
  _ContractAssert (x > 0);	/* { dg-warning "contract assertion cannot be proven" } */
  return 0;
}

int
i_weak_auto (int x, int y)
{
  h_weak_auto (y);
  if (x > 0)
    _ContractAssert (x > 0);	/* conditional */
  return 0;
}

/* { dg-final { scan-tree-dump {\(line 24\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 33\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 44\): unproven} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 53\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 64\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 73\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 84\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 93\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 104\): unproven} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 113\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 124\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 133\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 144\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 153\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 164\): unproven} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 173\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 184\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 193\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 204\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 213\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 224\): unproven} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 233\): conditional} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 244\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 253\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 264\): unproven} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 273\): conditional} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 284\): unproven} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 293\): conditional} "contract_shadow" } } */
