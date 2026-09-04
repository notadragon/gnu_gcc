// gcc-15-outlined-checks-lose-by-value-mutations.cpp                 -*-C++-*-
//
// GCC-15: under `-fcontract-checks-outlined', a contract predicate that
// mutates a BY-VALUE PARAMETER (or the result binding) writes to a copy, so
// the mutation is invisible to the function body and to the caller.  Without
// the flag the same program observes it.  The observable behaviour of a
// conforming program therefore depends on a codegen flag.
//
//   g++ -std=c++26 -fcontracts                            g15.cpp && ./a.out  -> 0
//   g++ -std=c++26 -fcontracts -fcontract-checks-outlined g15.cpp && ./a.out  -> 19
//
// MEASURED 2026-09-02, exactly which entities are affected:
//
//   | entity mutated in the predicate     | inlined  | outlined |
//   |-------------------------------------|----------|----------|
//   | global                              | observed | observed |
//   | BY-VALUE parameter                  | observed | **LOST** |
//   | reference parameter                 | observed | observed |
//   | result binding in a postcondition   | observed | **LOST** |
//   | local named in a `contract_assert'  | observed | observed |
//
// The pattern is exact: the entities that are lost are the ones passed BY
// VALUE to the outlined check function.  References survive because the
// reference is copied and the referent is shared; globals survive because
// they are not parameters at all.
//
// MECHANISM.  `build_contract_condition_function' (gcc/cp/contracts.cc)
// builds `__pre_fn'/`__post_fn' by copying the original function's parameter
// list verbatim -- `copy_decl' on each PARM_DECL, same types -- so a by-value
// `int n' is still by-value in the checking function and the caller hands it
// a copy.  For postconditions the result is appended as a further by-value
// parameter `__r' of the original return type.
//
// WHY [basic.contract.eval]'s ELISION LATITUDE DOES NOT EXCUSE THIS.
//
// That paragraph says a checking-semantic evaluation "determines the value of
// the predicate", that "it is unspecified whether the predicate is evaluated",
// and that "an alternative evaluation that produces the same value as the
// predicate but HAS NO SIDE EFFECTS can occur" -- with a normative example
// whose comment reads "Increment of s.g might not occur".  So a program may
// not rely on a predicate's side effects, and at first sight that covers this.
//
// It does not, because operating on a copy is not elision.  Eliding gives you
// ALL the side effects or NONE.  Copying gives you a PARTIAL application:
// mutations of shared state still propagate while mutations of the copied
// entity vanish, and the result is a state that neither permitted reading can
// produce.  Demonstrated by `partial_application' below:
//
//     faithful evaluation      -> 303
//     fully elided predicate   -> 200
//     g++ -fcontract-checks-outlined -> 203     <-- neither
//
// 203 is not "the same value with no side effects", and it is not the value
// with all of them.  There is no reading of the paragraph that produces it.
//
// THE BY-VALUE PARAMETER CASE IS THE CLEAR BUG.  A by-value parameter is one
// object in the function's frame; the precondition is evaluated after
// parameter initialization, and the body then reads that same object.  There
// is no license anywhere for the predicate to be handed a copy of it.  The
// first case below shows the body reading 2 where 3 is required.
//
// THE RESULT-BINDING CASE is the same defect reached through `__r', and is
// entangled with PR112794 (our GCC-12), which is independently a bug for a
// stronger reason: there the object confusion changes the predicate's VALUE,
// not merely its side effects.  Fix the parameter half here; make `__r' agree
// with whatever GCC-12 settles.
//
// PROVENANCE: UPSTREAM'S.  The upstream testsuite already carries the failing
// case, `g++.dg/contracts/cpp26/expr.prim.id.unqual.p7-3.C', marked
//
//     { dg-xfail-run-if "PRXXXXXX" { *-*-* } }
//
// -- a placeholder where a PR number should be.  So upstream knew about this
// and never filed it; there is no PR to comment on and nothing to duplicate.
// Our branch only replaced that placeholder with a description of the symptom.
//
// NOT contracts-extension-dependent: plain `-fcontracts' plus the upstream
// flag `-fcontract-checks-outlined', both of which predate this branch.

int g = 0;

// (1) By-value parameter: the body must see the precondition's increment.
//     This is the unambiguous half.
int
by_value_parm (int n) pre (const_cast<int &> (n)++)
{
  return n;                     // required: 3.  outlined gives 2.
}

// (2) Result binding: same shape, but see PR112794 before "fixing" it.
int
result_binding () post (r : const_cast<int &> (r)++)
{
  return 1;                     // caller sees 2 inlined, 1 outlined.
}

// (3) Control: a reference parameter survives, because the reference is what
//     is copied.
int
ref_parm (int &n) pre (const_cast<int &> (n)++)
{
  return n;
}

// (4) Control: a global survives, being no one's parameter.
void
global_mut () pre (const_cast<int &> (g)++)
{
}

int g_log = 0;

bool
bump (int &n)
{
  ++n;
  g_log = n;                    // shared state: propagates even when outlined
  return true;
}

// One predicate, two side effects -- one on a by-value parameter, one on a
// global.  Either both happen or neither may; outlining does exactly one.
int
partial_application (int n) pre (bump (const_cast<int &> (n)))
{
  return n * 100 + g_log;       // faithful 303, elided 200, outlined 203
}

int
main ()
{
  int bad = 0;

  {
    int r = partial_application (2);
    if (r != 303 && r != 200)
      bad |= 16;                // 203: neither faithful nor elided
  }

  if (by_value_parm (2) != 3)
    bad |= 1;                   // fires only when outlined

  if (result_binding () != 2)
    bad |= 2;                   // fires only when outlined

  int v = 2;
  ref_parm (v);
  if (v != 3)
    bad |= 4;                   // never fires

  g = 3;
  global_mut ();
  if (g != 4)
    bad |= 8;                   // never fires

  return bad;                   // inlined: 0.  outlined: 19 (16|2|1).
}
