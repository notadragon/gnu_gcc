// P3100 x P4298: implicit null-dereference (ub:expr.unary.dereference.nullptr)
// configured to "noexcept_enforce" via a NAMESPACE-SCOPED config match, built
// under -O2 -flto.  This is the LTO-specific instance of the null-deref
// regression class: at LTO stream-in (lto-streamer-in.cc), .UBSAN_NULL calls
// are rewritten to IFN_NOP when SANITIZE_NULL|SANITIZE_ALIGNMENT are off.  A
// contract null-check runs with -fsanitize=null OFF, so without a reaction-aware
// guard the check would be silently dropped at stream-in -> the handler never
// runs and the program dies with a raw SIGSEGV instead of reporting.
//
// The fix reads the P3100 reaction carried on .UBSAN_NULL operand 3 and refuses
// to NOP a check whose reaction is non-IMPLICIT_UB_NONE, so a contract check
// survives LTO regardless of the global sanitizer flags.  This test pins
// -O2 -flto and asserts the noexcept_enforce handler runs and reports the
// violation (sem=7) before the noreturn entry terminates.
//
// { dg-do run { target c++26 } }
// { dg-require-effective-target lto }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2 -flto -Wno-return-type" }
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
  int load_it (int *p) { return *p; }   // line 40: the dereference
}

int main ()
{
  int *p = nullptr;
  return deref_ns::load_it (p);
}

// The check survives LTO stream-in, the noexcept_enforce handler runs and
// reports the violation at -O2 -flto (sem=7 == noexcept_enforce), then the
// noreturn entry terminates the program before the dereference is reached.
// { dg-output "VIOL kind=7 sem=7 comment=\\\[null pointer dereference\\\] line=40" }
