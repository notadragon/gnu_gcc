/* P3400: a facet member must be const or static, and both must work.

   Facets are invoked on a constexpr -- therefore const -- assertion-control
   object, so a concept accepts the call when the member is const or static
   and rejects a non-const one.  The front end has to reach the same verdict,
   because __combined_label dispatches on the concepts: if the two disagree,
   combining a label changes its behaviour.

   Mirror of Clang's Runnable/p3400-facet-static-and-nonconst.cpp, where two
   bugs sat on opposite sides of this -- a static `query' crashed the compiler
   outright, and a non-const `handle_contract_violation' was accepted as a
   facet.  GCC has neither defect; nothing pinned that here.  The static
   `query' shape in particular had no GCC test at all, while static and
   non-const `handle_contract_violation' were already covered by
   p3400-facet-local-static.C and p3400-facet-near-miss.C.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe -Wno-contract-invalid-label-facet" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

using std::contracts::contract_violation;
using std::contracts::evaluation_semantic;
using std::contracts::violation_handled;
namespace lbl = std::contracts::labels;

static int key = 1;
static int failures = 0;
static bool query_ok = false;
static int local_calls = 0;

void handle_contract_violation (const contract_violation& v)
{ query_ok = v.query_control_object (&key, 0) != nullptr; }

static void
check (const char *what, bool ok)
{
  if (!ok)
    {
      std::printf ("FAIL: %s\n", what);
      ++failures;
    }
  query_ok = false;
  local_calls = 0;
}

/* --- static members are facets. ------------------------------------- */

/* This shape crashed Clang: detection built a member reference and called
   BuildCallToMemberFunction, which asserts on a callee of ordinary function
   type -- which is what a static member reference is.  */
struct static_query_t {
  using assertion_control_object = static_query_t;
  static void* query (const void* k, unsigned long)
  { return k == &key ? (void*) &key : nullptr; }
};
constexpr static_query_t static_query{};

struct static_handler_t {
  using assertion_control_object = static_handler_t;
  static violation_handled
  handle_contract_violation (const contract_violation&)
  { ++local_calls; return violation_handled::not_handled; }
};
constexpr static_handler_t static_handler{};

/* Static and non-static facets on one label.  */
struct static_both_t {
  using assertion_control_object = static_both_t;
  static void* query (const void* k, unsigned long)
  { return k == &key ? (void*) &key : nullptr; }
  violation_handled
  handle_contract_violation (const contract_violation&) const
  { ++local_calls; return violation_handled::not_handled; }
};
constexpr static_both_t static_both{};

static_assert (lbl::queryable_label<static_query_t>);
static_assert (lbl::local_violation_label<static_handler_t>);
static_assert (lbl::queryable_label<static_both_t>);
static_assert (lbl::local_violation_label<static_both_t>);

/* --- non-const members are not facets. ------------------------------ */

struct nonconst_query_t {
  using assertion_control_object = nonconst_query_t;
  void* query (const void*, unsigned long) { return (void*) &key; }
};
constexpr nonconst_query_t nonconst_query{};

struct nonconst_handler_t {
  using assertion_control_object = nonconst_handler_t;
  violation_handled handle_contract_violation (const contract_violation&)
  { ++local_calls; return violation_handled::handled; }
};
constexpr nonconst_handler_t nonconst_handler{};

static_assert (!lbl::queryable_label<nonconst_query_t>);
static_assert (!lbl::local_violation_label<nonconst_handler_t>);

/* --- the bare label and its combined form must agree. --------------- */

using lbl::operator|;
constexpr auto combined_static_query   = static_query   | lbl::empty_label;
constexpr auto combined_nonconst_query = nonconst_query | lbl::empty_label;
constexpr auto combined_nonconst_hcv   = nonconst_handler | lbl::empty_label;

static_assert ( lbl::queryable_label<decltype (combined_static_query)>);
static_assert (!lbl::queryable_label<decltype (combined_nonconst_query)>);
static_assert (!lbl::local_violation_label<decltype (combined_nonconst_hcv)>);

void f_static_query    (int x) pre<static_query>            (x > 0) { }
void f_static_handler  (int x) pre<static_handler>          (x > 0) { }
void f_static_both     (int x) pre<static_both>             (x > 0) { }
void f_nonconst_query  (int x) pre<nonconst_query>          (x > 0) { }
void f_nonconst_hcv    (int x) pre<nonconst_handler>        (x > 0) { }
void f_combined_static (int x) pre<combined_static_query>   (x > 0) { }

int
main ()
{
  query_ok = false; local_calls = 0;

  f_static_query (-1);
  check ("static query answers", query_ok);

  f_static_handler (-1);
  check ("static handler runs", local_calls == 1);

  f_static_both (-1);
  check ("static query + non-static handler",
	 query_ok && local_calls == 1);

  /* Not facets, so nothing of theirs runs.  A non-const handler that was
     wrongly treated as a facet would claim the violation and suppress the
     global handler; a non-const query would answer.  */
  f_nonconst_query (-1);
  check ("non-const query is not a facet", !query_ok);

  f_nonconst_hcv (-1);
  check ("non-const handler is not a facet", local_calls == 0);

  f_combined_static (-1);
  check ("combined static query answers", query_ok);

  return failures;
}
