// D4405 ("Controlling the Analysis Semantic with Labels"): the standard
// diagnose label sets only the analysis semantic, so built to observe, a
// precondition, a postcondition and a contract_assert with it each warn
// at compile time that they cannot be proven, and their runtime
// evaluations are still observe: each violation is reported and the
// program continues.  (p4405-std-labels-diagnose-enforce-run.C: built to
// enforce.)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405" }
// { dg-additional-options "-fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

static int violations;

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  ++violations;
  std::printf ("violation: %s, %s\n", v.comment (),
	       v.semantic () == std::contracts::evaluation_semantic::observe
	       ? "observe" : "other");
}

[[gnu::noipa]] void pre_d (int x) pre<diagnose> (x > 0) {}
[[gnu::noipa]] int post_d (int x) post<diagnose> (r: r > 0) { return x; }	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
[[gnu::noipa]] void assert_d (int x)
{
  contract_assert<diagnose> (x > 0);	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
}

int
main (int argc, char **)
{
  int n = argc - 2;
  pre_d (n);	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
  post_d (n);
  assert_d (n);
  std::printf ("after: %d\n", violations);
  if (violations != 3)
    __builtin_abort ();
}

// { dg-output "violation: x > 0, observe\[\r\n\]+" }
// { dg-output "violation: r > 0, observe\[\r\n\]+" }
// { dg-output "violation: x > 0, observe\[\r\n\]+" }
// { dg-output "after: 3" }
