// P3100 x P4298: implicit null-dereference (ub:expr.unary.dereference.nullptr)
// configured to "noexcept_enforce" via a NAMESPACE-SCOPED config match.  The
// callee that performs the dereference lives in its own namespace (deref_ns);
// main is in the global namespace and calls it with a null constant.
//
// This is the inlining-regression scenario: the reaction is resolved at
// pass_ubsan (pre-inline) where cfun->decl is deref_ns::load_it, so the
// namespace-scoped config matches.  At -O1/-O2/-O3 the callee is inlined into
// main (global namespace); if the reaction were re-resolved post-inline against
// main's namespace the config would no longer match, have_contract would go
// false, and the null check would be deleted -- producing a raw SIGSEGV instead
// of running the handler.  Carrying the reaction on the .UBSAN_NULL operand (its
// 4th operand, an enum implicit_ub_reaction) makes the CHECK survive inlining,
// so at -O2 the program now terminates through the check rather than a silent
// SIGSEGV -- the execution test below passes at every opt level.
//
// The handler is also inline-safe: the reaction operand carries the SPECIFIC
// semantic (noexcept_enforce vs noexcept_observe), and the handler-building
// langhook (cp_build_implicit_ub_handler) uses that carried reaction directly
// instead of re-resolving against the post-inline cfun->decl (main's
// namespace).  So at -O2 the noexcept_enforce handler actually runs and reports
// the violation (sem=7) before the noreturn entry terminates.
//
// The contracts harness runs each test once (no torture), so this file pins
// -O2 -- the level at which the regression manifests (the callee is inlined
// into main, deleting the check pre-fix).  The -O0/-O1/-O3/-Og variants are
// exercised manually during development; -O2 is the load-bearing regression
// guard committed here.
//
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2 -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-nullderef-opt.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_enforce reports then terminates" }

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
  // returns -> the noreturn noexcept_enforce entry terminates; deref not reached
}

namespace deref_ns
{
  int load_it (int *p) { return *p; }   // line 53: the dereference
}

int main ()
{
  int *p = nullptr;
  return deref_ns::load_it (p);
}

// The check survives inlining, the noexcept_enforce handler runs and reports
// the violation at -O2 (sem=7 == noexcept_enforce), then the noreturn entry
// terminates the program before the dereference is reached.
// { dg-output "VIOL kind=7 sem=7 comment=\\\[null pointer dereference\\\] line=53" }
