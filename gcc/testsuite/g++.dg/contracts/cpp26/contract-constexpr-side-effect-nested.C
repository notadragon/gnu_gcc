/* -Wcontract-constexpr-side-effect must survive a nested tracker.

   The warning fires only where modifiable_tracker REFUSED a modification, so
   anything that silently switches tracking off loses it.  A contract
   predicate that calls a constexpr function containing its own contract
   assertion starts a second tracker, and ~modifiable_tracker used to clear
   constexpr_global_ctx::modifiable rather than restoring the enclosing
   tracker's set -- so the rest of the outer predicate ran untracked, nothing
   was refused, and the diagnostic vanished.  The modification still happened,
   which is what made it silent: the program was as semantic-dependent as ever
   and simply stopped saying so.

   Same shapes as contract-constexpr-side-effect-warn.C, each preceded by
   something that opens and closes an inner tracker.  Enabled by default, so
   no -W option is passed.  */

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

constexpr bool bump (unsigned *p) { *p += 1; return true; }
constexpr bool clean (const unsigned *p) { return *p < 1000; }

/* An inner contract assertion: opens and closes a tracker of its own.  */
constexpr bool
inner_contract (const unsigned *p)
{
  contract_assert (clean (p));
  return true;
}

/* An inner [[assume]]: the other route to a nested tracker.  */
constexpr bool
inner_assume (const unsigned *p)
{
  [[assume (*p < 1000)]];
  return true;
}

/* The control, with no nesting -- this one always warned.  */
constexpr unsigned
plain (unsigned *p)
{
  contract_assert (bump (p)); // { dg-warning "contract predicate modifies .x., an object of the enclosing constant evaluation" }
  // { dg-message "not evaluated under the .ignore. semantic" "note" { target *-*-* } .-1 }
  return *p;
}

/* Nested contract assertion before the modification.  */
constexpr unsigned
after_contract (unsigned *p)
{
  contract_assert (inner_contract (p) && bump (p)); // { dg-warning "contract predicate modifies .x., an object of the enclosing constant evaluation" }
  // { dg-message "not evaluated under the .ignore. semantic" "note" { target *-*-* } .-1 }
  return *p;
}

/* Nested [[assume]] before the modification.  */
constexpr unsigned
after_assume (unsigned *p)
{
  contract_assert (inner_assume (p) && bump (p)); // { dg-warning "contract predicate modifies .x., an object of the enclosing constant evaluation" }
  // { dg-message "not evaluated under the .ignore. semantic" "note" { target *-*-* } .-1 }
  return *p;
}

/* Two frames deep, and the modification after both.  */
constexpr bool
two_deep (const unsigned *p)
{
  contract_assert (inner_contract (p));
  return true;
}

constexpr unsigned
after_two (unsigned *p)
{
  contract_assert (two_deep (p) && bump (p)); // { dg-warning "contract predicate modifies .x., an object of the enclosing constant evaluation" }
  // { dg-message "not evaluated under the .ignore. semantic" "note" { target *-*-* } .-1 }
  return *p;
}

/* Silent even with nesting: the predicate reads but does not modify.  */
constexpr unsigned
reads_only (unsigned *p)
{
  contract_assert (inner_contract (p) && clean (p));
  return *p;
}

constexpr unsigned run_plain () { unsigned x = 0; return plain (&x); } // { dg-message "in .constexpr. expansion of" }
constexpr unsigned run_after_contract () { unsigned x = 0; return after_contract (&x); } // { dg-message "in .constexpr. expansion of" }
constexpr unsigned run_after_assume () { unsigned x = 0; return after_assume (&x); } // { dg-message "in .constexpr. expansion of" }
constexpr unsigned run_after_two () { unsigned x = 0; return after_two (&x); } // { dg-message "in .constexpr. expansion of" }
constexpr unsigned run_reads_only () { unsigned x = 7; return reads_only (&x); }

static_assert (run_plain () == 1); // { dg-message "in .constexpr. expansion of" }
static_assert (run_after_contract () == 1); // { dg-message "in .constexpr. expansion of" }
static_assert (run_after_assume () == 1); // { dg-message "in .constexpr. expansion of" }
static_assert (run_after_two () == 1); // { dg-message "in .constexpr. expansion of" }
static_assert (run_reads_only () == 7);
