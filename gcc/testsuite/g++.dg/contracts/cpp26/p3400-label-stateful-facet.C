/* A P3400 facet whose result comes from the label's own state -- a data
   member rather than a template parameter or a static member -- must be
   honoured, including when the label also carries a facet that needs its
   address.

   Regression test.  A label carrying an address-needing facet (query or
   handle_contract_violation) is materialized into a TU-local constant, and
   that constant was built static but neither const, TREE_READONLY nor
   DECL_DECLARED_CONSTEXPR_P.  Constant evaluation therefore could not read
   it, so any facet consumed after materialization whose result depends on
   the object's state failed to fold -- and failed silently:
   compute_semantic_core saw error_mark_node and returned the semantic
   unchanged, with no diagnostic.  The assertion then ran with the build's
   default semantic instead of the label's.

   What matters is that the facet result is *state-dependent*.  A semantic
   from a template parameter or a static member folds without reading the
   object and always worked; so did a label with no address-needing facet,
   which is never materialized.  The form the label is written in --
   prvalue, named constexpr object, or the return of a constexpr factory --
   is irrelevant, and all four are checked here: a named object is
   materialized too, since the label expression is not a bare VAR_DECL.

   The string facets escape the bug because grok_contract applies them
   eagerly, before materialization; compute_comment is checked below so that
   stays true.  allowed_semantics is covered by
   p3400-label-stateful-allowed.C.

   Found by the BDE contracts integration, restructuring its labels onto a
   single type produced by factory functions carrying the semantic as a
   member.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

using std::contracts::contract_violation;
using std::contracts::evaluation_semantic;
using std::contracts::violation_handled;

static int global_ran = 0;
static int label_ran = 0;
static const char *seen_level = nullptr;

static constexpr int key = 0;

static void
note (const contract_violation &v)
{
  seen_level = (const char *) v.query_control_object (&key);
}

/* The build default is ignore, so every label below asking for observe must
   make a handler run.  If its semantic is dropped the contract is ignored
   and nothing runs -- silent, which is exactly the bug.  */

/* Member-held semantic, query facet.  */
struct MemQ
{
  using assertion_control_object = MemQ;
  evaluation_semantic sem;
  const char *level;
  constexpr MemQ (evaluation_semantic s, const char *l) : sem (s), level (l) {}
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return sem; }
  void *query (const void *k, __SIZE_TYPE__ i) const
  { return (i != 0 || k != &key) ? nullptr : const_cast<char *> (level); }
};

/* Member-held semantic, local-handler facet.  */
struct MemH
{
  using assertion_control_object = MemH;
  evaluation_semantic sem;
  constexpr MemH (evaluation_semantic s) : sem (s) {}
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return sem; }
  violation_handled handle_contract_violation (const contract_violation &) const
  { ++label_ran; return violation_handled::handled; }
};

/* Both address-needing facets at once.  */
struct MemQH
{
  using assertion_control_object = MemQH;
  evaluation_semantic sem;
  const char *level;
  constexpr MemQH (evaluation_semantic s, const char *l) : sem (s), level (l) {}
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return sem; }
  void *query (const void *k, __SIZE_TYPE__ i) const
  { return (i != 0 || k != &key) ? nullptr : const_cast<char *> (level); }
  violation_handled handle_contract_violation (const contract_violation &v) const
  { ++label_ran; note (v); return violation_handled::handled; }
};

/* Member-held semantic and a member-held comment, with a query facet.  The
   comment facet runs eagerly and was never broken; checked so it stays that
   way.  */
struct MemC
{
  using assertion_control_object = MemC;
  evaluation_semantic sem;
  const char *cmt;
  constexpr MemC (evaluation_semantic s, const char *c) : sem (s), cmt (c) {}
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return sem; }
  constexpr const char *compute_comment (const char *) const { return cmt; }
  void *query (const void *k, __SIZE_TYPE__ i) const
  { return (i != 0 || k != &key) ? nullptr : const_cast<char *> ("C"); }
};

/* Controls: the semantic does not come from the object, so folding never
   had to read it.  */
template <evaluation_semantic S>
struct TmplQ
{
  using assertion_control_object = TmplQ;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return S; }
  void *query (const void *k, __SIZE_TYPE__ i) const
  { return (i != 0 || k != &key) ? nullptr : const_cast<char *> ("T"); }
};

struct StatQ
{
  using assertion_control_object = StatQ;
  static constexpr evaluation_semantic sem = evaluation_semantic::observe;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return sem; }
  void *query (const void *k, __SIZE_TYPE__ i) const
  { return (i != 0 || k != &key) ? nullptr : const_cast<char *> ("S"); }
};

/* Control: member-held semantic but no facet needing an address, so the
   label is never materialized.  */
struct MemNone
{
  using assertion_control_object = MemNone;
  evaluation_semantic sem;
  constexpr MemNone (evaluation_semantic s) : sem (s) {}
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return sem; }
};

/* The four ways of writing the label.  */
constexpr MemQ named_q { evaluation_semantic::observe, "NAMED" };
constexpr MemH named_h { evaluation_semantic::observe };

struct Factory
{
  static constexpr MemQ q (evaluation_semantic s = evaluation_semantic::observe,
			   const char *l = "FACTORY")
  { return MemQ (s, l); }
  static constexpr MemH h (evaluation_semantic s = evaluation_semantic::observe)
  { return MemH (s); }
};

void
handle_contract_violation (const contract_violation &v)
{
  ++global_ran;
  note (v);
}

static void
reset () { global_ran = 0; label_ran = 0; seen_level = nullptr; }

static void
expect_global (const char *level)
{
  if (global_ran != 1 || label_ran != 0)
    __builtin_abort ();
  if (!seen_level || std::strcmp (seen_level, level) != 0)
    __builtin_abort ();
}

static void
expect_label ()
{
  if (label_ran != 1 || global_ran != 0)
    __builtin_abort ();
}

int
main ()
{
  /* Member-held semantic + query, in all four label forms.  */
  reset (); contract_assert<MemQ{evaluation_semantic::observe, "PRVALUE"}> (false);
  expect_global ("PRVALUE");

  reset (); contract_assert<named_q> (false);
  expect_global ("NAMED");

  reset (); contract_assert<Factory::q (evaluation_semantic::observe, "CALL")> (false);
  expect_global ("CALL");

  reset (); contract_assert<Factory::q ()> (false);
  expect_global ("FACTORY");

  /* Member-held semantic + local handler, in all four forms.  */
  reset (); contract_assert<MemH{evaluation_semantic::observe}> (false);
  expect_label ();

  reset (); contract_assert<named_h> (false);
  expect_label ();

  reset (); contract_assert<Factory::h (evaluation_semantic::observe)> (false);
  expect_label ();

  reset (); contract_assert<Factory::h ()> (false);
  expect_label ();

  /* Both facets on one label: the local handler takes the violation, and the
     query still answers from the materialized object.  */
  reset (); contract_assert<MemQH{evaluation_semantic::observe, "BOTH"}> (false);
  expect_label ();
  if (!seen_level || std::strcmp (seen_level, "BOTH") != 0)
    __builtin_abort ();

  /* The eagerly-applied comment facet still reads member state.  */
  reset (); contract_assert<MemC{evaluation_semantic::observe, "MEMBER-COMMENT"}> (false);
  expect_global ("C");

  /* Controls that always worked.  */
  reset (); contract_assert<TmplQ<evaluation_semantic::observe>{}> (false);
  expect_global ("T");

  reset (); contract_assert<StatQ{}> (false);
  expect_global ("S");

  reset (); contract_assert<MemNone{evaluation_semantic::observe}> (false);
  if (global_ran != 1 || label_ran != 0)
    __builtin_abort ();

  /* A label whose semantic resolves to ignore must still be ignored.  */
  reset (); contract_assert<MemQ{evaluation_semantic::ignore, "IGN"}> (false);
  if (global_ran != 0 || label_ran != 0)
    __builtin_abort ();

  return 0;
}
