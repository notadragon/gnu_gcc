// P3400: Mixed normal and contract_control using directives in lookup.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace labels_a {
  struct label_a_t {
    using assertion_control_object = label_a_t;
  };
  constexpr label_a_t label_a{};
}

namespace labels_b {
  struct label_b_t {
    using assertion_control_object = label_b_t;
  };
  constexpr label_b_t label_b{};
}

namespace utilities {
  constexpr int magic = 42;
}

// Normal using directive — makes utilities::magic visible everywhere.
using namespace utilities;

// Contract-control directives — make labels visible only in assertions.
using contract_control namespace labels_a;
using contract_control namespace labels_b;

// Both labels found through contract_control, magic through normal using.
void f(int x)
  pre<label_a>(x > 0)
  pre<label_b>(x < magic)
{
}

// Verify normal lookup still works alongside contract_control lookup.
int get_magic() { return magic; }

// contract_control(expr) also finds both namespaces.
constexpr auto copy_a = contract_control(label_a);
constexpr auto copy_b = contract_control(label_b);

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

int main() {
  f(-1);      // violates first pre (label_a)
  if (violations != 1) __builtin_abort();
  f(100);     // violates second pre (label_b, x < 42)
  if (violations != 2) __builtin_abort();
  f(10);      // passes both
  if (get_magic() != 42) __builtin_abort();
}
