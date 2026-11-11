/* P4405 premise copies in C (-fcontracts-prove-callee-premises,
   notadragon_wg21 DECISIONS.md E199 (b)): a C99 inline callee's observe
   precondition is a premise for the consumer through the callee's copy
   when the premise shadow inlines it (always_inline), so the consumer is
   proven conditionally; a noinline copy lends nothing.  Pinned to -O2.  */
/* Mirror: clang/test/Contracts/C/c-p4405-premise-semantic.c (its copy runs)  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -O2 -Wno-attributes -fdump-tree-contract_shadow" } */
/* { dg-additional-options "-fcontract-evaluation-semantic=observe -fcontract-configuration-file=${srcdir}/gcc.dg/contracts/c-p4405-premise-copies.json" } */
/* { dg-skip-if "pinned to -O2" { *-*-* } { "-O0" "-O1" "-O3" "-Os" "-Og" "-flto" } { "" } } */

int sink (int);

__attribute__ ((always_inline)) inline void h_always (int x) _Pre (x > 0) { sink (x); }

int
d_always (int x)
{
  h_always (x);
  _ContractAssert (x > 0);	/* { dg-bogus "cannot be proven" } */
  return 0;
}

__attribute__ ((noinline)) inline void h_never (int x) _Pre (x > 0) { sink (x); }

int
d_never (int x)
{
  h_never (x);
  _ContractAssert (x > 0);	/* { dg-warning "contract assertion cannot be proven" } */
  return 0;
}

/* { dg-final { scan-tree-dump {contract obligation [0-9]+ \(line 20\): unproven} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {callee premises: obligation [0-9]+ \(line 20\): proven, conditional} "contract_shadow" } } */
/* { dg-final { scan-tree-dump-not {callee premises: obligation [0-9]+ \(line 30\): proven} "contract_shadow" } } */
