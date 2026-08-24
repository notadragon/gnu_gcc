// P3100 x P4298: the -O2 inlining scenario of p3100-nullderef-opt.C, but with
// the implicit null-dereference (ub:expr.unary.dereference.nullptr) configured
// to "noexcept_observe" via a NAMESPACE-SCOPED config match.  The callee that
// performs the dereference lives in its own namespace (deref_ns); main is in the
// global namespace and calls it with a null constant, so at -O2 the callee is
// inlined into main (a different namespace).
//
// This is the companion to the enforce test: it proves the SPECIFIC semantic
// (noexcept_observe, distinct from noexcept_enforce) survives inlining.  The
// reaction operand carried on .UBSAN_NULL is IMPLICIT_UB_NOEXCEPT_OBSERVE, and
// the handler-building langhook uses that carried reaction directly rather than
// re-resolving against the post-inline cfun->decl (main's namespace).  So at
// -O2 the observe handler runs and reports sem=6 (noexcept_observe, NOT the
// enforce sem=7), and because the observe entry point RETURNS (it is not
// noreturn like enforce), execution CONTINUES after the handler returns and
// proceeds into the real (still-UB) null dereference -- "report then proceed".
// The program therefore crashes after the handler returns; dg-shouldfail
// confirms that continuation, and the dg-output line confirms the observe
// handler ran at -O2 with the correct static data.
//
// If the langhook re-resolved post-inline (the pre-fix bug), it would return
// false in main's namespace and ubsan_expand_null_ifn would emit __builtin_trap
// instead -- no VIOL line and a trap rather than the observe report.
//
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2 -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-nullderef-opt-observe.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_observe reports then proceeds into the null deref" }

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
  // returns -> noexcept_observe proceeds into the real (null) dereference
}

namespace deref_ns
{
  int load_it (int *p) { return *p; }   // line 48: the dereference
}

int main ()
{
  int *p = nullptr;
  return deref_ns::load_it (p);
}

// The observe handler runs at -O2 (sem=6 == noexcept_observe), then returns and
// execution continues into the raw null dereference (the program crashes after
// the report -- dg-shouldfail).
// { dg-output "VIOL kind=7 sem=6 comment=\\\[null pointer dereference\\\] line=48" }
