// P3400: Test compute_comment facet with various label types and orderings.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

using std::contracts::evaluation_semantic;
using std::contracts::labels::operator|;

// --- Test labels ---

struct redact_comment_t {
  using assertion_control_object = redact_comment_t;
  constexpr const char* compute_comment(const char*) const {
    return "[redacted]";
  }
};
constexpr redact_comment_t redact_comment{};

struct passthrough_comment_t {
  using assertion_control_object = passthrough_comment_t;
  constexpr const char* compute_comment(const char* c) const {
    return c;
  }
};
constexpr passthrough_comment_t passthrough_comment{};

static const char* last_comment = nullptr;
void handle_contract_violation(const std::contracts::contract_violation& v) {
  last_comment = v.comment();
}

// --- Functions under test ---

// No label
void f_plain(int x) pre(x > 0) { }

// Redact: replaces comment
void f_redact(int x) pre<redact_comment>(x > 0) { }

// Passthrough: leaves comment unchanged
void f_passthrough(int x) pre<passthrough_comment>(x > 0) { }

// Combined: redact | passthrough (redact first, passthrough identity)
void f_redact_pass(int x)
  pre<(redact_comment | passthrough_comment)>(x > 0)
{ }

// Combined: passthrough | redact (passthrough identity, then redact)
void f_pass_redact(int x)
  pre<(passthrough_comment | redact_comment)>(x > 0)
{ }

// Combined: passthrough | passthrough (both identity)
void f_pass_pass(int x)
  pre<(passthrough_comment | passthrough_comment)>(x > 0)
{ }

int main() {
  // Plain: comment is predicate text
  f_plain(-1);
  if (!last_comment || std::strcmp(last_comment, "x > 0") != 0)
    __builtin_abort();

  // Redact: comment replaced
  f_redact(-1);
  if (!last_comment || std::strcmp(last_comment, "[redacted]") != 0)
    __builtin_abort();

  // Passthrough: comment unchanged
  f_passthrough(-1);
  if (!last_comment || std::strcmp(last_comment, "x > 0") != 0)
    __builtin_abort();

  // redact | passthrough: redact first → "[redacted]", passthrough keeps it
  f_redact_pass(-1);
  if (!last_comment || std::strcmp(last_comment, "[redacted]") != 0)
    __builtin_abort();

  // passthrough | redact: passthrough keeps original, redact replaces
  f_pass_redact(-1);
  if (!last_comment || std::strcmp(last_comment, "[redacted]") != 0)
    __builtin_abort();

  // passthrough | passthrough: both identity, original kept
  f_pass_pass(-1);
  if (!last_comment || std::strcmp(last_comment, "x > 0") != 0)
    __builtin_abort();
}
