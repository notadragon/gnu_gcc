/* P3400: a facet member inherited from a base class is a facet.

   The concepts use ordinary member lookup, which sees inherited members, so
   the front end must too.  GCC has never had a defect here; this is the
   mirror of Clang's Runnable/p3400-facet-inherited.cpp, where detection
   agreed with the concepts but the trampoline builder scanned direct members
   only, so an inherited handler or query was detected and then never called.

   The offsets are as much the point as the inheritance.  A trampoline that
   passes the label pointer straight through as `this' is only correct when
   the member belongs to the most-derived class or to a base at offset zero;
   under multiple inheritance the base sits further along and the pointer has
   to be stepped to it.  An unadjusted `this' still calls the right function,
   just on the wrong bytes -- so each facet below reads a member of its own
   base, which is the only way a missing adjustment shows up.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

using std::contracts::contract_violation;
using std::contracts::evaluation_semantic;
using std::contracts::violation_handled;
namespace lbl = std::contracts::labels;

static int key = 1;
static int failures = 0;
static int observed_tag = 0;
static int global_calls = 0;
static bool query_ok = false;

void handle_contract_violation (const contract_violation& v)
{
  ++global_calls;
  query_ok = v.query_control_object (&key, 0) != nullptr;
}

static void
check (const char *what, int tag, int want_tag, bool want_query)
{
  if (tag != want_tag || query_ok != want_query)
    {
      std::printf ("FAIL: %s: tag %d (want %d), query %d (want %d)\n",
		   what, tag, want_tag, (int) query_ok, (int) want_query);
      ++failures;
    }
  observed_tag = 0;
  query_ok = false;
  global_calls = 0;
}

/* Bases carrying each facet, each reading a member of its own.  */

struct HandlerBase {
  int tag = 4242;
  violation_handled
  handle_contract_violation (const contract_violation&) const
  { observed_tag = tag; return violation_handled::not_handled; }
};

struct QueryBase {
  int qtag = 777;
  void* query (const void* k, unsigned long) const
  { return (k == &key && qtag == 777) ? (void*) &key : nullptr; }
};

struct CommentBase {
  constexpr const char* compute_comment (const char*) const
  { return "inherited-comment"; }
};

/* Single inheritance, base at offset zero.  */
struct SingleHandler : HandlerBase {
  using assertion_control_object = SingleHandler;
};
constexpr SingleHandler single_handler{};

struct SingleQuery : QueryBase {
  using assertion_control_object = SingleQuery;
};
constexpr SingleQuery single_query{};

struct SingleComment : CommentBase {
  using assertion_control_object = SingleComment;
};
constexpr SingleComment single_comment{};

/* Multiple inheritance: Pad pushes both facet bases off offset zero.  */
struct Pad { long long a = 0x1111, b = 0x2222; };

struct MultiBoth : Pad, HandlerBase, QueryBase {
  using assertion_control_object = MultiBoth;
};
constexpr MultiBoth multi_both{};

/* A facet inherited two levels down.  */
struct Middle : HandlerBase { };
struct TwoLevel : Pad, Middle {
  using assertion_control_object = TwoLevel;
};
constexpr TwoLevel two_level{};

/* The concepts must agree with all of the above.  */
static_assert (lbl::local_violation_label<SingleHandler>);
static_assert (lbl::queryable_label<SingleQuery>);
static_assert (lbl::compute_comment_label<SingleComment>);
static_assert (lbl::local_violation_label<MultiBoth>);
static_assert (lbl::queryable_label<MultiBoth>);
static_assert (lbl::local_violation_label<TwoLevel>);

void f_single_handler (int x) pre<single_handler> (x > 0) { }
void f_single_query   (int x) pre<single_query>   (x > 0) { }
void f_single_comment (int x) pre<single_comment> (x > 0) { }
void f_multi_both     (int x) pre<multi_both>     (x > 0) { }
void f_two_level      (int x) pre<two_level>      (x > 0) { }

int
main ()
{
  observed_tag = 0; query_ok = false; global_calls = 0;

  f_single_handler (-1);
  check ("single handler", observed_tag, 4242, false);

  f_single_query (-1);
  check ("single query", 0, 0, true);

  /* The comment facet is consumed during translation, so it never reaches a
     trampoline; it is here as the control that says so.  */
  f_single_comment (-1);
  check ("single comment", 0, 0, false);

  /* Both facets inherited, both at non-zero offsets.  */
  f_multi_both (-1);
  check ("multiple inheritance", observed_tag, 4242, true);

  f_two_level (-1);
  check ("two levels, non-zero offset", observed_tag, 4242, false);

  return failures;
}
