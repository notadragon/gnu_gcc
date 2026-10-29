# GCC-152: a predicate that exits via an exception is reported as predicate_false

**Status:** Open (upstream); fixed here
**Component:** c++ (contracts), libstdc++
**Keywords (ours -- upstream sets its own):** `wrong-code`
**Upstream Link:** [PR125163](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125163)
(UNCONFIRMED; filed by someone else 2026-05-03).  Its comment 2 reports a
second, unrelated defect, the quick_enforce warning that is GCC-689.
**Affects:** measured 2026-10-10: 16.1.0, 16.2.0 and stock trunk
(`17.0.0 20261005`, the CE nightly at `8802cd9fdbe`) print `mode=1`; this
branch prints `mode=2`.

**Bugzilla Summary:** When a contract predicate exits via exception the
contract violation shows detection_mode as predicate_false

## Reproducer

[`gcc-152-throwing-predicate-detection-mode.cpp`](gcc-152-throwing-predicate-detection-mode.cpp),
at `-std=c++26 -fcontracts -fcontract-evaluation-semantic=observe`, built
and run:

    bool g (int x) { if (x == 0) throw std::runtime_error ("throwing"); return true; }
    void f (int x) pre (g (x)) { }
    int main () { f (0); }

with a handler that prints the violation's detection mode.  Stock prints
`mode=1` (`predicate_false`); the expected value is `mode=2`
(`evaluation_exception`): [support.contract.violation] says
`detection_mode()` returns "the enumerator value corresponding to the manner
in which the contract violation was identified", and the table of
[support.contract.enum] (`tab:support.contract.enum.detection`) defines
`evaluation_exception` as "An uncaught exception occurred during evaluation
of the contract assertion".

## Notes

Stock `<contracts>` also spells the accessor `mode()`, where
[support.contract.violation] declares
`contracts::detection_mode detection_mode() const noexcept;` -- the PR's
first comment notices this too.  The reproducer probes for either spelling
with a requires-expression, so the one file compiles against both
compilers.  This branch declares `detection_mode()`.

On this branch the exception path calls a separate libcontracts entry point
that encodes the detection mode in its name (`gcc/cp/contracts.cc`,
`emit_check_for_semantic`, the `CDM_EVAL_EXCEPTION` entry point), so no
violation object has to be rewritten after the fact; the stock code path
that copies or rewrites the violation object is gone.  No single commit is
about this defect.

The PR's later comment (2026-06-20) suggests `evaluation_exception` is no
longer wanted, citing P3819R0.  That paper removed the
`evaluation_exception()` *member function* (the `exception_ptr` accessor);
the `detection_mode::evaluation_exception` enumerator is still in the draft
([contracts.syn], `evaluation_exception = 2`), so the report stands.

## Found by

Review round #12's bug-trail reconciliation (2026-09-25), which measured the
PR on both compilers; recorded here 2026-10-10.
