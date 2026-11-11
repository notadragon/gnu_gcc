// P3400: queryable_label combined with local_violation_label (full data block).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

using std::contracts::contract_violation;
using std::contracts::violation_handled;

static int local_calls = 0;
static int global_calls = 0;

struct dual_label_t {
  using assertion_control_object = dual_label_t;
  static constexpr int info_key = 1;
  const char* info;

  constexpr dual_label_t(const char* i) : info(i) {}

  void* query(const void* key, std::size_t index) const {
    if (index == 0 && key == &info_key) return (void*)info;
    return nullptr;
  }

  violation_handled
  handle_contract_violation(const contract_violation& v) const {
    ++local_calls;
    // Verify query works from within local handler too.
    auto* r = v.query_control_object(&info_key);
    if (!r) __builtin_abort();
    if (std::strcmp((const char*)r, "dual-info") != 0) __builtin_abort();
    return violation_handled::handled;
  }
};

constexpr dual_label_t dual_label("dual-info");

void test_dual(int x) pre<dual_label>(x > 0) { }

void handle_contract_violation(const contract_violation&) {
  ++global_calls;
}

int main() {
  test_dual(-1);
  // Local handler was called and handled; global NOT called.
  if (local_calls != 1) __builtin_abort();
  if (global_calls != 0) __builtin_abort();
  return 0;
}
