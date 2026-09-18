// gcc-05-calendar-gimplify-ice.cpp                                   -*-C++-*-
//
// Found 2026-08-24 during the same `-k`/keep-going clean GCC build as GCC-4
// (assert2pre `bsl`/`bdl`/`bal` migration, `dbg_64_cpp26_contracts` ufid).
// Tracked internally as GCC-5.  MINIMIZED 2026-08-24 (see below) -- the entry
// below is corrected from the original write-up in two ways: the "function
// at the crash site" quote named the WRONG overload (see note), and only
// ONE precondition turns out to be needed, not several.
//
// GCC-5: a NON-TEMPLATE member function with a precondition, returning a
// class type with a user-declared (non-trivial) destructor, ICEs in the
// GIMPLE backend when its body has two loops shaped so that one exits via
// `break` (falling through to a `return` after the loop) and the other
// exits via a `return` written directly inside the loop body.
//
//   internal compiler error: in gimple_add_tmp_var, at gimplify.cc:845
//
// A DIFFERENT CRASH STAGE from GCC-4 and GCC-3 (both front-end/template
// substitution): this one is past semantic analysis and template
// instantiation entirely, in the GIMPLE lowering pass -- consistent with
// `Calendar` being an ordinary (non-template) class, which rules out any
// explanation involving template substitution.
//
// NEEDS NO EXTENSION OF OURS: reproduces with plain `-fcontracts` (no
// `-fcontracts-p3850`), at `-std=c++23` as well as `-std=c++26`.  Like
// GCC-4, `-fsyntax-only` alone compiles clean -- the crash needs the
// function to be emitted (`-c`).  Contracts are in GCC trunk, so per the
// provenance checklist used to confirm this is upstream's, not ours, this
// is filable.  PROVENANCE
// SETTLED 2026-08-25: upstream's, not ours -- both halves of the mechanism
// are byte-identical in trunk (`master:gcc/cp/contracts.cc:1372` and
// `master:gcc/cp/except.cc:1379`).  Still not run against a trunk build or
// public Compiler Explorer, which the checklist also asks for before filing.
//
// CORRECTION TO THE ORIGINAL WRITE-UP: the function quoted there
// (`getNextBusinessDay(Date*, const Date&, int)`, four `BSLS_PRE` clauses,
// `bdlt_calendar.h:958-963`) is a DIFFERENT overload from the one that
// actually crashes.  The real crash site, `bdlt_calendar.cpp:396`, is:
//
//   // bdlt_calendar.h:1263
//   Date getNextBusinessDay(const Date& initialDate, int nth) const
//       BSLS_PRE(nth >= 1);
//
// exactly ONE precondition.  Removing the P3400 label that `BSLS_PRE`
// expands to (testing with a bare `pre(nth >= 1)` in its place) still
// crashes identically, so the label/facet machinery is not implicated
// either -- this is a plain, unlabelled contract.
//
// MINIMIZATION, narrowed by isolating each ingredient against the real
// `bdlt::Date`/`bdlt::Calendar` translation unit before dropping BDE
// entirely: the essential ingredients are (1) the enclosing member
// function carries a precondition, (2) the return type is a class with a
// user-declared destructor (trivial-by-default is NOT enough -- an
// implicitly-defaulted destructor does not reproduce it), and (3) the
// function body contains two loop constructs that exit by different
// means: one via `break` with the `return` written after the loop, the
// other via a `return` written directly inside the loop body.  Two loops
// that both return directly from inside (dropping the break-then-return
// shape) do NOT reproduce it.  The precondition's own content is
// irrelevant, and neither the destructor nor any other member needs a
// contract of its own -- only a user-declared (even empty-bodied)
// destructor on the return type.
//
// FIXED 2026-08-25, gnu_gcc `6e5a47c2a39`; test
// `g++.dg/contracts/cpp26/contract-retval-sentinel.C`.  See
// `gcc-05-contract-retval-double-destroy.md` in this directory for the full
// write-up, the provenance finding (upstream's -- both halves are
// byte-identical in trunk) and the still-owed upstream filing.  TWO
// CORRECTIONS to the analysis below, both established 2026-08-25:
//
//   * "NOT YET LOCALIZED: which binding scope contracts push/pop
//     differently" -- the question was wrong.  Contracts push nothing
//     unusual.  `do_poplevel` (`cp/semantics.cc:663-666`) calls `poplevel`
//     BEFORE `maybe_splice_retval_cleanup`, so by the time the latter runs,
//     the artificial block's own level is already gone and the enclosing
//     `sk_function_parms` is current again.  Any block popped directly at
//     that level looks like the function body, and the contracts wrapper is
//     one.  That is the whole mechanism; no scope is misclassified.
//
//   * The ICE below is not the only symptom, and is the less serious one.
//     Where the returned object is NOT a named return value there is no
//     assertion to trip, and the duplicated cleanup silently destroys the
//     returned object twice -- a clean compile and a corrupted program.  See
//     `gcc-05b-contract-retval-double-destroy.cpp`, which is the reproducer
//     to lead an upstream report with.
//
// The fix hides `current_retval_sentinel` across the wrapper block, so
// `maybe_splice_retval_cleanup` takes its early exit for it -- the
// idempotence-guard direction guessed at below, but placed in
// `cp/contracts.cc` rather than in `maybe_splice_retval_cleanup`, which would
// also have reached function-try-blocks and the c++/112301 rethrow path.
//
// ROOT CAUSE, PARTIALLY LOCALIZED 2026-08-24 (mechanism identified, exact
// fix not yet found -- superseded by the note above): `gimple_add_tmp_var`'s
// assertion
// (`!DECL_CHAIN (tmp) && !DECL_SEEN_IN_BIND_EXPR_P (tmp)`) fires because
// the SAME `current_retval_sentinel` VAR_DECL (`cp/except.cc`, a
// per-function boolean guard created once by `maybe_set_retval_sentinel`
// to track whether a named-return-value object needs destroying on an
// exception path) gets a `DECL_EXPR` inserted into the function's
// statement list TWICE by `maybe_splice_retval_cleanup`
// (`cp/except.cc:1357-1358`), each into a DIFFERENT nested scope --
// confirmed with `gdb` (breaking on `maybe_splice_retval_cleanup` against
// a debug build): it is called once from `do_poplevel` <-
// `cp_parser_compound_statement` (finishing the body's own `{ }` braces)
// and once more from `do_poplevel` <- `finish_function_body` (finishing
// the outer function-body scope) -- both calls happen for EVERY function
// with a non-trivial-destructor return type (confirmed: `Date`'s own
// ctors/dtor/`operator++` all hit both call sites too), so ordinarily only
// ONE of the two sees `current_binding_level->kind == sk_function_parms`
// (`maybe_splice_retval_cleanup`'s `function_body` test) and inserts the
// `DECL_EXPR`.  Something about a contract's presence on the function
// makes BOTH calls see `sk_function_parms`, so both insert -- confirmed by
// the genericized tree (`-fdump-tree-original`) showing `bool D.NNNN = 0;`
// declared twice, once in the precondition-check's wrapping block and
// once inside the real body's block.  NOT YET LOCALIZED: which binding
// scope contracts push/pop differently, and whether the fix belongs in
// `cp/contracts.cc`'s body-wrapping or in `maybe_splice_retval_cleanup`
// itself (e.g. guard against inserting the `DECL_EXPR` a second time for
// the same `current_retval_sentinel`, mirroring the idempotence pattern
// already used elsewhere in contract-adjacent GCC code).

struct Date {
    int d;
    Date() : d(0) {}
    Date(int x) : d(x) {}
    Date(const Date& o) : d(o.d) {}
    ~Date() {}                                   // non-trivial: required
    Date& operator++() { ++d; return *this; }
    bool operator<(const Date& o) const { return d < o.d; }
};

struct Calendar {
    bool flag;
    bool weekend(Date) const { return false; }
    Date first() const { return Date(0); }

    // g++ -std=c++26 -fcontracts -c gcc-05-calendar-gimplify-ice.cpp
    //   -> internal compiler error: in gimple_add_tmp_var, at gimplify.cc:845
    Date f(const Date& initialDate, int nth) const pre(nth >= 1);
};

Date Calendar::f(const Date& initialDate, int nth) const
{
    Date currentDate = initialDate;
    ++currentDate;

    if (flag) {
        // Loop 1: exits via `break`, falls through to the `return` below.
        while (1) {
            if (!weekend(currentDate)) {
                --nth;
                if (0 == nth) {
                    break;
                }
            }
            ++currentDate;
        }
        return currentDate;
    }

    Date calendarFirstDate = first();

    // Loop 2: exits via a `return` written directly inside the loop body.
    while (currentDate < calendarFirstDate) {
        if (!weekend(currentDate)) {
            --nth;
            if (0 == nth) {
                return currentDate;
            }
        }
        ++currentDate;
    }

    return currentDate;
}

int main()
{
    Calendar c{true};
    c.f(Date(1), 1);
}
