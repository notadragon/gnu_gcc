// P3400: Test compute_message facet with various label types and orderings.
// Tests both with and without an explicit message in the assertion.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontracts-p3099 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

using std::contracts::evaluation_semantic;
using std::contracts::labels::operator|;
using std::contracts::labels::fixed_message_label_t;

// --- Test labels ---

struct redact_message_t {
  using assertion_control_object = redact_message_t;
  constexpr const char* compute_message(const char*) const {
    return "[message redacted]";
  }
};
constexpr redact_message_t redact_message{};

struct passthrough_message_t {
  using assertion_control_object = passthrough_message_t;
  constexpr const char* compute_message(const char* m) const {
    return m;
  }
};
constexpr passthrough_message_t passthrough_message{};

static const char* last_message = nullptr;
static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation& v) {
  last_message = v.message();
  ++violations;
}

// =============================
// Functions WITHOUT explicit message
// =============================

void f_no_msg_plain(int x) pre(x > 0) { }

void f_no_msg_redact(int x) pre<redact_message>(x > 0) { }

void f_no_msg_passthrough(int x) pre<passthrough_message>(x > 0) { }

void f_no_msg_redact_pass(int x)
  pre<(redact_message | passthrough_message)>(x > 0)
{ }

void f_no_msg_pass_redact(int x)
  pre<(passthrough_message | redact_message)>(x > 0)
{ }

void f_no_msg_fixed(int x)
  pre<fixed_message_label_t{"fixed msg"}>(x > 0)
{ }

// =============================
// Functions WITH explicit message
// =============================

void f_msg_plain(int x) pre(x > 0, "explicit msg") { }

void f_msg_redact(int x) pre<redact_message>(x > 0, "explicit msg") { }

void f_msg_passthrough(int x)
  pre<passthrough_message>(x > 0, "explicit msg")
{ }

void f_msg_redact_pass(int x)
  pre<(redact_message | passthrough_message)>(x > 0, "explicit msg")
{ }

void f_msg_pass_redact(int x)
  pre<(passthrough_message | redact_message)>(x > 0, "explicit msg")
{ }

void f_msg_fixed(int x)
  pre<fixed_message_label_t{"fixed msg"}>(x > 0, "explicit msg")
{ }

int main() {
  // === Without explicit message ===

  // Plain: no message → null
  f_no_msg_plain(-1);
  // (null message is implementation-defined, don't check exact value)

  // Redact: no explicit message → facet receives null, returns "[message redacted]"
  f_no_msg_redact(-1);
  if (!last_message || std::strcmp(last_message, "[message redacted]") != 0)
    __builtin_abort();

  // Passthrough: no explicit message → facet receives null, returns null
  f_no_msg_passthrough(-1);
  // passthrough returns what it receives (null)

  // redact | passthrough: redact → "[message redacted]", passthrough keeps it
  f_no_msg_redact_pass(-1);
  if (!last_message || std::strcmp(last_message, "[message redacted]") != 0)
    __builtin_abort();

  // passthrough | redact: passthrough null→null, redact → "[message redacted]"
  f_no_msg_pass_redact(-1);
  if (!last_message || std::strcmp(last_message, "[message redacted]") != 0)
    __builtin_abort();

  // fixed_message_label_t: always returns "fixed msg"
  f_no_msg_fixed(-1);
  if (!last_message || std::strcmp(last_message, "fixed msg") != 0)
    __builtin_abort();

  // === With explicit message ===

  // Plain with msg: message is "explicit msg"
  f_msg_plain(-1);
  if (!last_message || std::strcmp(last_message, "explicit msg") != 0)
    __builtin_abort();

  // Redact with msg: facet replaces with "[message redacted]"
  f_msg_redact(-1);
  if (!last_message || std::strcmp(last_message, "[message redacted]") != 0)
    __builtin_abort();

  // Passthrough with msg: facet returns "explicit msg" unchanged
  f_msg_passthrough(-1);
  if (!last_message || std::strcmp(last_message, "explicit msg") != 0)
    __builtin_abort();

  // redact | passthrough: redact → "[message redacted]", passthrough keeps it
  f_msg_redact_pass(-1);
  if (!last_message || std::strcmp(last_message, "[message redacted]") != 0)
    __builtin_abort();

  // passthrough | redact: passthrough keeps "explicit msg", redact replaces
  f_msg_pass_redact(-1);
  if (!last_message || std::strcmp(last_message, "[message redacted]") != 0)
    __builtin_abort();

  // fixed with msg: fixed always returns "fixed msg"
  f_msg_fixed(-1);
  if (!last_message || std::strcmp(last_message, "fixed msg") != 0)
    __builtin_abort();
}
