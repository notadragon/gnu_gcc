// D4405R0, "P3400R5 Analysis Evaluation Configuration": the diagnosed label
// of p4405-paper-analysis-facet.C built to ignore contract assertions: the
// analysis evaluation still warns, and the runtime evaluation is ignore,
// so f(-1) reports nothing.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>  // `evaluation_semantic`, `contract_violation`
#include <cstdio>     // `std::printf`, `std::fflush`, `std::puts`

// The paper's diagnosed label.
struct diagnosed_t {
  using assertion_control_object = diagnosed_t;

  constexpr std::contracts::evaluation_semantic analysis_semantic() const {
    return std::contracts::evaluation_semantic::diagnose;
  }
};
constexpr diagnosed_t diagnosed{};

// A violation handler that reports each violation; with *enforce* the
// program then terminates.
void handle_contract_violation(const std::contracts::contract_violation& v)
{
  std::printf("violation: %s\n", v.comment());
  std::fflush(stdout);
}

void f(int x)
{
  contract_assert<diagnosed>(x > 0);  // OK, warns: cannot be proven	{ dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
}

int main()
{
  f(-1);
  std::puts("done");
}

// { dg-output "done" }
