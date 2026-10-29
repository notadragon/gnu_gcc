/* PR c++/125725 -- an attribute-specifier-seq on a postcondition result name
   was rejected.

     result-name-introducer:
       identifier attribute-specifier-seq[opt] :

   Only the plain `identifier :' spelling was recognized, so
   `post (r [[maybe_unused]]: c)' fell through and was parsed as the start of
   the predicate, producing a cascade of five unrelated errors beginning with
   "'r' was not declared in this scope".

   KNOWN LIMITATION, tested here so it cannot regress into a silent drop
   unnoticed: on an in-class contract the predicate is deferred and the result
   variable is built later, from the identifier stored on the contract node,
   which has nowhere to carry the attributes -- so an attribute there is
   accepted and then ignored.  Accepted is the point; the attributes that make
   sense on a result name are advisory.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

static int viol = 0;
static int failures = 0;
void handle_contract_violation (const std::contracts::contract_violation&)
{ ++viol; }

static void
check (const char *what, int want)
{
  if (viol != want)
    {
      std::printf ("FAIL: %s: expected %d, got %d\n", what, want, viol);
      ++failures;
    }
  viol = 0;
}

/* The reported shape, at namespace scope.  */
int f (int x) post (r [[maybe_unused]]: r > 0) { return x; }

/* Two attributes, and the GNU spelling.  The [[deprecated]] warning is the
   proof that the attributes are APPLIED and not merely parsed and dropped --
   which is the whole difference between fixing this and papering over it.  */
int g (int x) post (r [[maybe_unused, deprecated]]: r > 0) { return x; } // { dg-warning "'r' is deprecated" }
int h (int x) post (r __attribute__((unused)): r > 0) { return x; }

/* In-class: deferred, and must at least be accepted.  */
struct S {
  int m (int x) post (r [[maybe_unused]]: r > 0) { return x; }
};

/* Templates, both spellings.  */
template<typename T> T t (T x) post (r [[maybe_unused]]: r > 0) { return x; }
struct U { template<typename T> T m (T x) post (r [[maybe_unused]]: r > 0)
	     { return x; } };

/* Controls that must keep working: no attribute, and -- the shape a careless
   lookahead would break -- a predicate that merely BEGINS with an identifier,
   with no result-name-introducer at all.  */
int plain (int x) post (r : r > 0) { return x; }
int no_intro (int x) pre (x > 0) { return x; }

int
main ()
{
  viol = 0;

  if (f (1) != 1) __builtin_abort ();
  check ("namespace scope, satisfied", 0);
  f (-1);
  check ("namespace scope, violated", 1);

  if (g (1) != 1) __builtin_abort ();
  check ("two attributes", 0);
  if (h (1) != 1) __builtin_abort ();
  check ("GNU attribute spelling", 0);

  S s;
  if (s.m (1) != 1) __builtin_abort ();
  check ("in-class (deferred), satisfied", 0);
  s.m (-1);
  check ("in-class (deferred), violated", 1);

  if (t (1) != 1) __builtin_abort ();
  check ("function template", 0);
  U u;
  if (u.m (1) != 1) __builtin_abort ();
  check ("member template", 0);

  if (plain (1) != 1) __builtin_abort ();
  check ("no attribute control", 0);
  if (no_intro (1) != 1) __builtin_abort ();
  check ("no result-name-introducer control", 0);

  return failures;
}
