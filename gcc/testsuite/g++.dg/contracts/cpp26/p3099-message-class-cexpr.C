// P3099: a custom-type (non-literal) diagnostic message on a *member-function*
// (late-parsed) postcondition/precondition is normalized to a bare STRING_CST
// and reaches the handler.  Regression test for BUG-3's sibling manifestation:
// GCC ICEd ("expected string_cst, have var_decl in
// build_contract_data_block_ctor") because the late-parse path stored the raw
// message without the normalization grok_contract applies to non-deferred
// contracts.  Complements p3099-message-cexpr-str.C (free function) and
// p3099-message-class.C (member function, string-literal message).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3099 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

struct MyMessage {
  const char* str;
  constexpr int size() const { return __builtin_strlen(str); }
  constexpr const char* data() const { return str; }
};
constexpr MyMessage msg{"member custom msg"};

static const char* last = nullptr;
void handle_contract_violation(const std::contracts::contract_violation& v) {
  last = v.message();
}

struct Widget {
  int val = -1;
  void set_value(int x) pre(x > 0, msg) { val = x; }
  int get_value() const post(r: r >= 0, msg) { return val; }
};

int main() {
  Widget w;
  w.set_value(-1);   // precondition fails -> custom message
  if (!last || std::strcmp(last, "member custom msg") != 0) __builtin_abort();
  last = nullptr;
  (void) w.get_value();  // postcondition fails (val == -1) -> custom message
  if (!last || std::strcmp(last, "member custom msg") != 0) __builtin_abort();
}
