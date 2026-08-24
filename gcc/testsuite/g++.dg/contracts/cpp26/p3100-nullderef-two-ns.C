// P3100 x P4298: two implicit null-dereference checks
// (ub:expr.unary.dereference.nullptr) on the SAME pointer, configured with
// DIFFERENT reactions via NAMESPACE-SCOPED config matches.  This exercises the
// sanopt UBSAN_NULL redundancy-elimination path (gcc/sanopt.cc,
// maybe_optimize_ubsan_null_ifn).
//
// obs_ns::load_it dereferences p under "noexcept_observe" (reaction
// IMPLICIT_UB_NOEXCEPT_OBSERVE, reports sem=6 then RETURNS/continues);
// enf_ns::load_it dereferences the SAME p under "noexcept_enforce" (reaction
// IMPLICIT_UB_NOEXCEPT_ENFORCE, reports sem=7 then the noreturn entry
// terminates).  At -O2 both callees are inlined into main and both .UBSAN_NULL
// checks land in one function on the same (null-constant) pointer, so the
// redundancy pass sees a dominating check for the second site.
//
// Because the two checks carry DIFFERENT reactions (operand 3 of .UBSAN_NULL,
// added in Task 1), they are NOT equivalent and must NOT be merged: each site
// keeps its own reaction.  The redundancy pass reports "Optimizing out:" only
// when it drops a redundant check; with distinct reactions it must drop none.
// If the reaction were ignored (the bug this guards), the dominated second
// check would be dropped -- the enforce site would silently vanish and inherit
// the observe reaction.
//
//   scan-tree-dump-times "Optimizing out" 0 -- neither differently-reacting
//   check is merged away (compile-time proof each site keeps its reaction).
//
// The run half is a live end-to-end check that the FIRST-reached site fires its
// own (observe, sem=6) reaction and continues -- then the program crashes at
// that site's real null dereference (dg-shouldfail).  Both halves together show
// each site keeps its own reaction across inlining + redundancy elimination.
//
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2 -Wno-return-type -fdump-tree-sanopt-details" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-nullderef-two-ns.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-skip-if "" { *-*-* } { "-flto -fno-fat-lto-objects" } }
// { dg-shouldfail "observe reports+continues into the real null deref" }

#include <contracts>
#include <cstdio>

namespace cs = std::contracts;

void handle_contract_violation (const cs::contract_violation& v)
{
  auto loc = v.location ();
  std::printf ("VIOL kind=%d sem=%d comment=[%s] line=%u\n",
	       (int) v.kind (), (int) v.semantic (), v.comment (),
	       (unsigned) loc.line ());
  std::fflush (stdout);
}

namespace obs_ns
{
  int load_it (int *p) { return *p; }   // line 54: observe deref (sem=6)
}

namespace enf_ns
{
  int load_it (int *p) { return *p; }   // line 59: enforce deref (sem=7)
}

int main ()
{
  int *p = nullptr;
  int a = obs_ns::load_it (p);          // reports sem=6, returns, continues
  int b = enf_ns::load_it (p);          // reached only if the observe deref
  return a + b;                         // did not crash first
}

// The first-reached (observe, sem=6) site fires its own reaction; the second
// (enforce) site keeps its distinct reaction and is NOT merged away.
// { dg-output "VIOL kind=7 sem=6 comment=\\\[null pointer dereference\\\] line=54" }
// { dg-final { scan-tree-dump-times "Optimizing out" 0 "sanopt" } }
