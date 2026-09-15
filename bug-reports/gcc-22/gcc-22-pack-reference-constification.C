// GCC-22 / PR126878, PR126039: a function parameter pack of reference type
// used in a contract assertion is rejected outright.
//
// Extracted from the fix commit's own DejaGnu test,
// gcc/testsuite/g++.dg/contracts/cpp26/pr126878.C (added by gnu_gcc commit
// ea65bf85250), found via `git show ea65bf85250 --stat` and read with
// `git show ea65bf85250:gcc/testsuite/g++.dg/contracts/cpp26/pr126878.C`.
// DejaGnu directive lines were stripped; the C++ source (including the
// test's own commentary comment block) was kept as-is. Requires
// -fcontracts -fcontract-evaluation-semantic=observe (C++26), linked with
// -lstdc++exp, and a hosted <contracts> header.

/* PR c++/126878 -- a function parameter PACK of reference type used in a
   contract assertion was rejected:

     error: 'const' qualifiers cannot be applied to 'Args&'

   PR c++/126039 is the same bug reached through a generic lambda's
   forwarding-reference pack (auto&&...), so it is covered here too.

   Contract access is constified through view_as_const, which cv-qualifies the
   accessed type.  A reference cannot be cv-qualified -- but it never needed
   to be: a non-pack reference parameter arrives already dereferenced, as a
   REFERENCE_REF whose type is the referent's, so it constifies the referent
   and never reaches the reference itself.  A pack element is still a bare
   PARM_DECL of type `T&' at that point, because the pack has not been
   expanded, so it did reach it and the hard error rejected a well-formed
   program.  Each element is constified the ordinary way once expanded.  */

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
      std::printf ("FAIL: %s: expected %d violations, got %d\n",
		   what, want, viol);
      ++failures;
    }
  viol = 0;
}

/* The reported shapes: a pack of lvalue references, in pre and in post.  */
template<typename... Args> void pre_pack (Args&... args)
  pre ((... && (args > 0))) { }

template<typename... Args> int post_pack (Args&... args)
  post (r : r == (0 + ... + args)) { return (0 + ... + args); }

/* A pack of references also used through a pack-index-expression.  */
template<typename... Args> void pre_index (Args&... args)
  pre (args...[0] > 0) { }

/* Forwarding references, the same thing spelled with a deduced type.  */
template<typename... Args> void pre_fwd (Args&&... args)
  pre ((... && (args > 0))) { }

/* PR c++/126039: the generic-lambda spelling.  */
auto lambda_fwd = [](auto&&... args) pre ((... && (args > 0)))
  { return (0 + ... + args); };

/* Controls that must keep working: a by-value pack, a single reference
   parameter, and a non-template reference parameter.  A by-value pack in a
   POSTcondition must still be const -- that rule is unaffected.  */
template<typename... Args> void pre_value (Args... args)
  pre ((... && (args > 0))) { }
template<typename T> void pre_one_ref (T& a) pre (a > 0) { }
void pre_plain_ref (int& a) pre (a > 0) { }
template<typename... Args> int post_const_value (const Args... args)
  post (r : r == (0 + ... + args)) { return (0 + ... + args); }

int
main ()
{
  int a = 1, b = 2, c = 3;
  int z = 0;

  pre_pack (a, b, c);
  check ("pack of references in pre, all satisfied", 0);
  pre_pack (a, z);
  check ("pack of references in pre, one violated", 1);

  if (post_pack (a, b, c) != 6) __builtin_abort ();
  check ("pack of references in post", 0);

  pre_index (a, b);
  check ("pack-index on a pack of references", 0);
  pre_index (z, b);
  check ("pack-index on a pack of references, violated", 1);

  pre_fwd (1, 2, 3);
  check ("forwarding-reference pack", 0);
  pre_fwd (1, 0);
  check ("forwarding-reference pack, violated", 1);

  if (lambda_fwd (1, 2, 3) != 6) __builtin_abort ();
  check ("generic lambda forwarding-reference pack", 0);
  lambda_fwd (1, 0);
  check ("generic lambda forwarding-reference pack, violated", 1);

  pre_value (1, 2);
  check ("by-value pack control", 0);
  pre_one_ref (a);
  check ("single reference parameter control", 0);
  pre_plain_ref (a);
  check ("non-template reference parameter control", 0);
  if (post_const_value (1, 2) != 3) __builtin_abort ();
  check ("const by-value pack in a postcondition control", 0);

  return failures;
}
