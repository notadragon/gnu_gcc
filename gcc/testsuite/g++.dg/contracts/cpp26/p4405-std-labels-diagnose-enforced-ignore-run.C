// D4405, E198 (d): diagnose-then-enforce is the standard diagnose label
// combined with a label whose compute_semantic returns enforce (written
// here, not in <contracts>).  The combination leaves diagnose allowed, so
// pre<diagnose | enforced> warns at compile time that it cannot be proven,
// and its runtime evaluation is enforce whatever the build configures --
// here ignore: f(-1) is a violation and the program terminates after the
// handler returns.  (p4405-std-labels-diagnose-enforced-observe-run.C:
// built to observe; p4405-std-labels.C: diagnose | always_enforce and
// diagnose | terminating are not diagnosed.)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "enforce terminates after the violation" }

#include <contracts>
#include <cstdio>

using std::contracts::evaluation_semantic;
using std::contracts::labels::operator|;

struct enforced_t {
  using assertion_control_object = enforced_t;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return evaluation_semantic::enforce; }
};
constexpr enforced_t enforced{};

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("violation: %s, %s\n", v.comment (),
	       v.semantic () == evaluation_semantic::enforce
	       ? "enforce" : "other");
  std::fflush (stdout);
}

[[gnu::noipa]] void f (int x) pre<diagnose | enforced> (x > 0) {}

int
main (int argc, char **)
{
  f (argc - 2);	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
  std::puts ("after");
}

// { dg-output "violation: x > 0, enforce" }
