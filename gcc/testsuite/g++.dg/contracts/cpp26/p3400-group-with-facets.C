// P3400: Test group labels combined with other facets (allowed_semantics,
// compute_comment, local_violation_label).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-group-evaluation-semantic=safety:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using std::contracts::labels::operator|;

// allowed_semantics label: only observe allowed
struct only_observe_t {
  using assertion_control_object = only_observe_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::observe};
};
constexpr only_observe_t only_observe{};

// redact comment label
struct redact_comment_t {
  using assertion_control_object = redact_comment_t;
  constexpr const char* compute_comment(const char*) const {
    return "[redacted]";
  }
};
constexpr redact_comment_t redact_comment{};

static int violations = 0;
static const char* last_comment = nullptr;
void handle_contract_violation(const std::contracts::contract_violation& v) {
  ++violations;
  last_comment = v.comment();
}

// Group + allowed_semantics: safety group observe, allowed only observe
void f_group_allowed(int x)
  pre<("safety"group | only_observe)>(x > 0)
{ }

// Group + compute_comment: safety group observe, comment redacted
void f_group_comment(int x)
  pre<("safety"group | redact_comment)>(x > 0)
{ }

// All three combined
void f_triple(int x)
  pre<("safety"group | only_observe | redact_comment)>(x > 0)
{ }

int main() {
  // Group + allowed_semantics: safety:observe, only_observe allows it
  f_group_allowed(-1);
  if (violations != 1) __builtin_abort();

  // Group + compute_comment: safety:observe, comment redacted
  f_group_comment(-1);
  if (violations != 2) __builtin_abort();
  if (!last_comment || std::strcmp(last_comment, "[redacted]") != 0)
    __builtin_abort();

  // Triple combination
  f_triple(-1);
  if (violations != 3) __builtin_abort();
  if (!last_comment || std::strcmp(last_comment, "[redacted]") != 0)
    __builtin_abort();
}
