/* -Wcontract-constexpr-side-effect: a contract predicate that modifies an
   object of the enclosing constant evaluation is well formed and the
   modification is performed (see contract-constexpr-side-effect.C), but the
   program then means different things under different evaluation semantics
   -- under ignore the predicate is not evaluated at all and the modification
   never happens.  Say so.

   Enabled by default, so no -W option is passed here.  The warning is once
   per contract location: the loop below evaluates one contract many times at
   compile time, and must produce one warning.  */

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

constexpr bool bump (unsigned *p) { *p += 1; return true; }
constexpr bool clean (const unsigned *p) { return *p < 1000; }

/* Warns: modifies the caller's object.  */
constexpr unsigned
modifies (unsigned *p)
{
  /* The object named is the one actually modified -- the caller's x -- not
     the pointer parameter it was reached through.  */
  contract_assert (bump (p)); // { dg-warning "contract predicate modifies .x., an object of the enclosing constant evaluation" }
  // { dg-message "not evaluated under the .ignore. semantic" "note" { target *-*-* } .-1 }
  return *p;
}

constexpr unsigned
run_modifies ()
{
  unsigned x = 0;
  return modifies (&x); // { dg-message "in .constexpr. expansion of" }
}

static_assert (run_modifies () == 1); // { dg-message "in .constexpr. expansion of" }

/* Evaluated many times in one constant evaluation, still one warning.  */
constexpr unsigned
loop_modifies ()
{
  unsigned x = 0;
  for (int i = 0; i < 20; ++i)
    modifies (&x);
  return x;
}

static_assert (loop_modifies () == 20);

/* Silent: the predicate reads but does not modify.  */
constexpr unsigned
reads_only (unsigned *p)
{
  contract_assert (clean (p));
  return *p;
}

constexpr unsigned
run_reads_only ()
{
  unsigned x = 7;
  return reads_only (&x);
}

static_assert (run_reads_only () == 7);

/* Silent: a predicate re-calling a constexpr function that the caller already
   called.  This used to be refused by the same machinery the warning keys on
   (see contract-constexpr-repeat-call.C), so it is a direct guard against the
   warning regressing into a false positive.  */
struct view
{
  const char *p;
  unsigned len;
  constexpr bool empty () const { return len == 0; }
  constexpr void advance () { ++p; --len; }
};

constexpr void
step (view *v)
{
  contract_assert (!v->empty ());
  v->advance ();
}

constexpr unsigned
run_repeat ()
{
  view v { "xyz", 3 };
  unsigned n = 0;
  while (!v.empty ())
    {
      step (&v);
      ++n;
    }
  return n;
}

static_assert (run_repeat () == 3);

/* Silent: modification at run time only, never constant-evaluated.  The
   warning deliberately does not reach this.  */
unsigned
runtime_only (unsigned *p)
{
  contract_assert (bump (p));
  return *p;
}
