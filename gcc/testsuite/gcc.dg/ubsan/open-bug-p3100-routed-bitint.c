/* WATCH TEST for GCC-867 (gnu_gcc open-issues/): the output tests XFAIL
   until it is fixed.  With -fsanitize=signed-integer-overflow routed to
   the contract-violation handler with noexcept_observe, a signed overflow
   of a _BitInt of more than 128 bits is not reported to the handler (0):
   libubsan reports it as an unsigned integer overflow (stock, its type
   descriptor being unknown to the runtime), a check that is not routed;
   and an overflowing multiplication of a _BitInt(127) is reported twice
   (2): the lowered check calls __ubsan_handle_mul_overflow twice (stock,
   where the runtime prints the second report of a location no more).  */
/* { dg-do run { target bitint } } */
/* { dg-additional-options "-std=c23 -fcontracts-p4299 -fcontracts-p3100 -fcontracts-p4298" } */
/* { dg-additional-options "-fsanitize=signed-integer-overflow -fsanitize-semantic=signed-integer-overflow:noexcept_observe" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */
/* { dg-bogus ".-fcontracts-p3100. is valid for C\\+\\+" "" { target *-*-* } 0 } */
/* { dg-bogus ".-fcontracts-p4298. is valid for C\\+\\+" "" { target *-*-* } 0 } */
/* { dg-bogus "has no effect without" "" { target *-*-* } 0 } */

#include <contracts.h>
#include <stdio.h>

static int reports;

void
handle_contract_violation (const contract_violation_t *cv)
{
  (void) cv;
  ++reports;
}

#if __BITINT_MAXWIDTH__ >= 135
__attribute__((noinline)) _BitInt(135)
addh (_BitInt(135) a, _BitInt(135) b)
{
  return a + b;
}

__attribute__((noinline)) _BitInt(127)
mull (_BitInt(127) a, _BitInt(127) b)
{
  return a * b;
}
#endif

int
main (void)
{
#if __BITINT_MAXWIDTH__ >= 135
  volatile _BitInt(135) hmx = 21778071482940061661655974875633165533183wb;
  volatile _BitInt(135) hone = 1;
  volatile _BitInt(127) lmx = 85070591730234615865843651857942052863wb;
  int before = reports;
  volatile _BitInt(135) h = addh (hmx, hone);
  printf ("addh %d\n", reports - before);
  before = reports;
  volatile _BitInt(127) l = mull (lmx, lmx);
  printf ("mull %d\n", reports - before);
  (void) h;
  (void) l;
#else
  printf ("addh 1\nmull 1\n");
#endif
  return 0;
}

/* { dg-output "addh 1(\n|\r\n|\r)" { xfail *-*-* } } */
/* { dg-output "mull 1(\n|\r\n|\r)" { xfail *-*-* } } */
