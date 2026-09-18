// P3400: Test compute_comment with define_static_string for dynamic strings.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe -freflection" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <meta>
#include <string>
#include <cstring>

using std::contracts::labels::operator|;

struct append_comment_t {
  using assertion_control_object = append_comment_t;
  const char* _M_suffix;
  constexpr append_comment_t(const char* __s) : _M_suffix(__s) {}
  consteval const char* compute_comment(const char* __c) const {
    std::string __s(__c);
    __s += _M_suffix;
    return std::define_static_string(__s);
  }
};

struct redact_comment_t {
  using assertion_control_object = redact_comment_t;
  constexpr const char* compute_comment(const char*) const {
    return "[redacted]";
  }
};
constexpr redact_comment_t redact_comment{};

static const char* last_comment = nullptr;
void handle_contract_violation(const std::contracts::contract_violation& v) {
  last_comment = v.comment();
}

// Append " [checked]" to predicate text
void f_append(int x) pre<append_comment_t{" [checked]"}>(x > 0) { }

// Append then redact: append first → "x > 0 [checked]", then redact
void f_append_redact(int x)
  pre<(append_comment_t{" [checked]"} | redact_comment)>(x > 0)
{ }

// Redact then append: redact first → "[redacted]", then append
void f_redact_append(int x)
  pre<(redact_comment | append_comment_t{" [checked]"})>(x > 0)
{ }

int main() {
  // Append: "x > 0" + " [checked]" = "x > 0 [checked]"
  f_append(-1);
  if (!last_comment || std::strcmp(last_comment, "x > 0 [checked]") != 0)
    __builtin_abort();

  // append | redact: append first, then redact replaces → "[redacted]"
  f_append_redact(-1);
  if (!last_comment || std::strcmp(last_comment, "[redacted]") != 0)
    __builtin_abort();

  // redact | append: redact first → "[redacted]", then append
  // → "[redacted] [checked]"
  f_redact_append(-1);
  if (!last_comment
      || std::strcmp(last_comment, "[redacted] [checked]") != 0)
    __builtin_abort();
}
