/* P3100: with -fsanitize=signed-integer-overflow routed to the
   contract-violation handler with noexcept_observe, the code after a
   signed-overflow check of a _BitInt that reports and continues sees the
   handler's side effects, in the function holding the check and in its
   caller (GCC-858): the check, an IFN_UBSAN_CHECK_* internal call, is not
   const when routed (gimple_call_flags), and _BitInt lowering replaces it
   with a call of the __ubsan_handle_* function, not a leaf call when
   routed.  A _BitInt of more than 64 bits (lowered) and one of fewer, an
   addition and a subtraction, in functions that are not inlined and in the
   caller, at -O2.  (A _BitInt of more than 128 bits, and a multiplication,
   are GCC-867's.)  */
/* { dg-do run { target bitint } } */
/* { dg-additional-options "-std=c23 -fcontracts-p4299 -fcontracts-p3100 -fcontracts-p4298 -O2" } */
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

#if __BITINT_MAXWIDTH__ >= 127
#define NI __attribute__((noinline))
NI _BitInt(127) addl (_BitInt(127) a, _BitInt(127) b) { return a + b; }
NI _BitInt(127) subl (_BitInt(127) a, _BitInt(127) b) { return a - b; }
NI _BitInt(31) adds (_BitInt(31) a, _BitInt(31) b) { return a + b; }
NI _BitInt(31) subs (_BitInt(31) a, _BitInt(31) b) { return a - b; }

#define COUNT(name, expr)				\
  do							\
    {							\
      int before = reports;				\
      expr;						\
      printf (name " %d\n", reports - before);		\
    }							\
  while (0)
#endif

int
main (void)
{
#if __BITINT_MAXWIDTH__ >= 127
  volatile _BitInt(127) lmx = 85070591730234615865843651857942052863wb;
  volatile _BitInt(127) lmn = -lmx - 1, lone = 1;
  volatile _BitInt(31) smx = 1073741823wb, smn = -smx - 1, sone = 1;
  volatile _BitInt(127) l;
  volatile _BitInt(31) s;
  COUNT ("addl", l = addl (lmx, lone));
  COUNT ("subl", l = subl (lmn, lone));
  COUNT ("adds", s = adds (smx, sone));
  COUNT ("subs", s = subs (smn, sone));
  COUNT ("iaddl", l = lmx + lone);
  COUNT ("isubl", l = lmn - lone);
  COUNT ("iadds", s = smx + sone);
  COUNT ("isubs", s = smn - sone);
  (void) l;
  (void) s;
#else
  printf ("addl 1\nsubl 1\nadds 1\nsubs 1\n");
  printf ("iaddl 1\nisubl 1\niadds 1\nisubs 1\n");
#endif
  return 0;
}

/* { dg-output "addl 1(\n|\r\n|\r)subl 1(\n|\r\n|\r)adds 1(\n|\r\n|\r)" } */
/* { dg-output "subs 1(\n|\r\n|\r)iaddl 1(\n|\r\n|\r)isubl 1(\n|\r\n|\r)" } */
/* { dg-output "iadds 1(\n|\r\n|\r)isubs 1(\n|\r\n|\r)" } */
