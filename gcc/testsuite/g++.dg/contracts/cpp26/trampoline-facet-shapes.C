// Six facet shapes that satisfy local_violation_label / queryable_label are
// each called exactly once: an explicit object parameter by value, an extra
// defaulted parameter, a member template, a result convertible to
// violation_handled, query(const void*, unsigned), and
// query(const void*, auto).
//
// Mirror: clang/test/Contracts/Runnable/trampoline-*.cpp (one file per shape)
// in the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;

static int local = 0;
static int key;

namespace s1 {
struct lab_t {
  using assertion_control_object = lab_t;
  void handle_contract_violation (this lab_t, const contract_violation&)
  { ++local; }
};
constexpr lab_t lab{};
void f (int x) pre<lab> (x > 0) {}
}

namespace s2 {
struct lab_t {
  using assertion_control_object = lab_t;
  void handle_contract_violation (const contract_violation&, int k = 1) const
  { local += k; }
};
constexpr lab_t lab{};
void f (int x) pre<lab> (x > 0) {}
}

namespace s3 {
struct lab_t {
  using assertion_control_object = lab_t;
  template <class V> void handle_contract_violation (const V&) const
  { ++local; }
};
constexpr lab_t lab{};
void f (int x) pre<lab> (x > 0) {}
}

namespace s4 {
struct result_t {
  operator std::contracts::violation_handled () const { return {}; }
};
struct lab_t {
  using assertion_control_object = lab_t;
  result_t handle_contract_violation (const contract_violation&) const
  { ++local; return {}; }
};
constexpr lab_t lab{};
void f (int x) pre<lab> (x > 0) {}
}

namespace s5 {
struct lab_t {
  using assertion_control_object = lab_t;
  void *query (const void *k, unsigned idx) const
  { ++local; return k == &key && idx == 0 ? &key : nullptr; }
};
constexpr lab_t lab{};
void f (int x) pre<lab> (x > 0) {}
}

namespace s6 {
struct lab_t {
  using assertion_control_object = lab_t;
  void *query (const void *k, auto idx) const
  { ++local; return k == &key && idx == 0 ? &key : nullptr; }
};
constexpr lab_t lab{};
void f (int x) pre<lab> (x > 0) {}
}

void handle_contract_violation (const contract_violation& v)
{
  (void) v.query_control_object (&key, 0);
}

int main ()
{
  s1::f (-1); if (local != 1) __builtin_abort ();
  s2::f (-1); if (local != 2) __builtin_abort ();
  s3::f (-1); if (local != 3) __builtin_abort ();
  s4::f (-1); if (local != 4) __builtin_abort ();
  s5::f (-1); if (local != 5) __builtin_abort ();
  s6::f (-1); if (local != 6) __builtin_abort ();
}
