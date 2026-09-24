// P3400: facet trampolines generated while an enclosing function body is
// being parsed.  A `contract_assert` is grokked in the middle of a function
// body, and a `post` is grokked with the body's parse state live, so building
// a facet trampoline at that point must not disturb it.  The pre-only tests
// nearby never exercised this.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::violation_handled;

static int local_calls = 0;
static int query_calls = 0;
static int statements_run = 0;

static constexpr int key = 7;
static int queried_value = 99;

// local_violation_label facet.
struct handler_label_t {
  using assertion_control_object = handler_label_t;
  violation_handled handle_contract_violation (const contract_violation&) const
  {
    ++local_calls;
    return violation_handled::handled;
  }
};
constexpr handler_label_t handler_label{};

// queryable_label facet.
struct query_label_t {
  using assertion_control_object = query_label_t;
  violation_handled handle_contract_violation (const contract_violation& v) const
  {
    if (v.query_control_object (&key) != &queried_value) __builtin_abort ();
    ++query_calls;
    return violation_handled::handled;
  }
  void *query (const void *k, __SIZE_TYPE__) const
  {
    return k == &key ? &queried_value : nullptr;
  }
};
constexpr query_label_t query_label{};

// The trampoline for handler_label_t is built while this body is being
// parsed.  The statements on either side must survive that.
void assert_in_body (int x)
{
  ++statements_run;
  contract_assert<handler_label> (x > 0);
  ++statements_run;
}

// A second contract_assert with the same label reuses the trampoline rather
// than building another.
void assert_reuses_trampoline (int x)
{
  contract_assert<handler_label> (x > 0);
  ++statements_run;
}

// A distinct label builds a second trampoline, again mid-body.
void assert_second_label (int x)
{
  contract_assert<query_label> (x > 0);
  ++statements_run;
}

// post, whose predicate is grokked with the enclosing definition's parse
// state live.
int post_with_facet (int x) post (r : r > 0)
{
  return x;
}

int post_with_label (int x) post<handler_label> (r : r > 0)
{
  return x;
}

// A postcondition result name raises processing_template_decl while the
// predicate is grokked; the trampoline built there is not a template.  This
// declaration is the first use of its label, so it is the one that builds it.
struct decl_label_t {
  using assertion_control_object = decl_label_t;
  violation_handled handle_contract_violation (const contract_violation&) const
  {
    ++local_calls;
    return violation_handled::handled;
  }
};
constexpr decl_label_t decl_label{};

int post_result_name_first_use (int x) post<decl_label> (r : r > 0);
int post_result_name_first_use (int x) { return x; }

// A labelled contract_assert nested inside a lambda body, which is itself
// inside a function body -- two levels of function context to preserve.
void assert_in_lambda (int x)
{
  auto f = [](int y) { contract_assert<handler_label> (y > 0); };
  f (x);
  ++statements_run;
}

void handle_contract_violation (const contract_violation&)
{
  // Only post_with_facet, which carries no label, reaches the global
  // handler.
  ++statements_run;
}

int main ()
{
  assert_in_body (-1);
  if (local_calls != 1 || statements_run != 2) __builtin_abort ();

  assert_reuses_trampoline (-1);
  if (local_calls != 2 || statements_run != 3) __builtin_abort ();

  assert_second_label (-1);
  if (query_calls != 1 || statements_run != 4) __builtin_abort ();

  post_with_facet (-1);
  if (statements_run != 5) __builtin_abort ();

  post_with_label (-1);
  if (local_calls != 3 || statements_run != 5) __builtin_abort ();

  assert_in_lambda (-1);
  if (local_calls != 4 || statements_run != 6) __builtin_abort ();

  post_result_name_first_use (-1);
  if (local_calls != 5 || statements_run != 6) __builtin_abort ();
}
