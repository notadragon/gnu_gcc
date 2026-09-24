// P3400: queryable_label facet -- basic query_control_object usage.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

using std::contracts::contract_violation;

struct my_query_label {
  using assertion_control_object = my_query_label;
  static constexpr int key1 = 1;
  static constexpr int key2 = 2;
  const char* value1;
  const char* value2;

  constexpr my_query_label(const char* v1, const char* v2)
  : value1(v1), value2(v2) {}

  void* query(const void* key, std::size_t index) const {
    if (index != 0) return nullptr;
    if (key == &key1) return (void*)value1;
    if (key == &key2) return (void*)value2;
    return nullptr;
  }
};

constexpr my_query_label labeled("hello", "world");

static int handler_calls = 0;

void test_basic(int x) pre<labeled>(x > 0) { }

void handle_contract_violation(const contract_violation& v) {
  ++handler_calls;

  // Query for key1 at index 0 -- should return "hello".
  auto* r1 = v.query_control_object(&my_query_label::key1);
  if (!r1) __builtin_abort();
  if (std::strcmp((const char*)r1, "hello") != 0) __builtin_abort();

  // Query for key2 at index 0 -- should return "world".
  auto* r2 = v.query_control_object(&my_query_label::key2);
  if (!r2) __builtin_abort();
  if (std::strcmp((const char*)r2, "world") != 0) __builtin_abort();

  // Query for key1 at index 1 -- should return nullptr.
  auto* r3 = v.query_control_object(&my_query_label::key1, 1);
  if (r3 != nullptr) __builtin_abort();

  // Query for unknown key -- should return nullptr.
  static constexpr int unknown_key = 99;
  auto* r4 = v.query_control_object(&unknown_key);
  if (r4 != nullptr) __builtin_abort();
}

int main() {
  test_basic(-1);
  if (handler_calls != 1) __builtin_abort();
  return 0;
}
