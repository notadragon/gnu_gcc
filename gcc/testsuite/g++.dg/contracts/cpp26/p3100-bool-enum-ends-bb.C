// P3100: a routed bool/enum representation check must be emitted even when
// the load ends its basic block.
//
// Regression test: instrument_bool_enum_load_contract bailed outright on
// stmt_ends_bb_p, before the configured reaction was even consulted,
// leaving the raw load in place -- that is, behaving as assume no matter
// what semantic was configured, including the defined-value substitution
// that ignore requires.  The stock instrument_bool_enum_load has always
// handled the case, by retargeting the load and inserting on the
// fallthrough edge.
//
// The trigger needs no exotic construction: under -fnon-call-exceptions
// any load inside an EH region can throw and so ends its block, and an EH
// region is created by something as ordinary as a local with a
// destructor.  No try/catch is required.
//
// The enum must NOT have a fixed underlying type -- with one, precision
// equals the mode precision and no check is eligible at all.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fnon-call-exceptions -O1 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-bool-enum-ends-bb.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct Guard { ~Guard (); };

enum E { A = 0, B = 1 };

/* The load ends its block: it sits in the EH region created by g.  */
bool
load_cleanup (const bool *p)
{
  Guard g;
  return *p;
}

/* Control: the same load with no EH region around it.  */
bool
load_plain (const bool *p)
{
  return *p;
}

E
enum_cleanup (const E *p)
{
  Guard g;
  return *p;
}

E
enum_plain (const E *p)
{
  return *p;
}

/* One per function.  Before the fix only the two plain forms were
   instrumented.  */
// { dg-final { scan-tree-dump-times "__builtin_trap" 4 "optimized" } }
