// P3400: queryable_label facet -- combined labels with binary search.
// Tests both associativity directions produce the same index ordering.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

using std::contracts::contract_violation;
using std::contracts::labels::operator|;

struct owner_label {
  using assertion_control_object = owner_label;
  static constexpr int name_key = 1;
  const char* name;

  constexpr owner_label(const char* n) : name(n) {}

  void* query(const void* key, std::size_t index) const {
    if (index == 0 && key == &name_key) return (void*)name;
    return nullptr;
  }
};

constexpr owner_label alice("Alice");
constexpr owner_label bob("Bob");
constexpr owner_label carol("Carol");

// (alice | bob) | carol -- left-associated
void test_left(int x) pre<(alice | bob) | carol>(x > 0) { }

// alice | (bob | carol) -- right-associated
void test_right(int x) pre<alice | (bob | carol)>(x > 0) { }

static int test_phase = 0;

static void validate_order(const contract_violation& v) {
  // All three names accessible in left-to-right order.
  auto* r0 = v.query_control_object(&owner_label::name_key, 0);
  if (!r0) __builtin_abort();
  if (std::strcmp((const char*)r0, "Alice") != 0) __builtin_abort();

  auto* r1 = v.query_control_object(&owner_label::name_key, 1);
  if (!r1) __builtin_abort();
  if (std::strcmp((const char*)r1, "Bob") != 0) __builtin_abort();

  auto* r2 = v.query_control_object(&owner_label::name_key, 2);
  if (!r2) __builtin_abort();
  if (std::strcmp((const char*)r2, "Carol") != 0) __builtin_abort();

  // Index 3 -- past the end.
  auto* r3 = v.query_control_object(&owner_label::name_key, 3);
  if (r3 != nullptr) __builtin_abort();
}

void handle_contract_violation(const contract_violation& v) {
  ++test_phase;
  validate_order(v);
}

int main() {
  test_left(-1);
  if (test_phase != 1) __builtin_abort();

  test_right(-1);
  if (test_phase != 2) __builtin_abort();

  return 0;
}
