# Open Upstream Contracts PRs That Are Not Ours

The correlation sweep of 2026-09-05 walked every open PR in GCC Bugzilla with
"contracts" in the summary -- 33 of them -- and measured each against stock
g++ trunk (17.0.0 20260901) and this branch. The ones that turned out to
correlate with our work are rows in [`README.md`](README.md). **This file
records the rest, so the same PRs are not investigated again.**

A PR listed here can still become relevant: "not ours" is a measurement, and
measurements go stale. Each entry says what was measured, so a future sweep
can tell whether the conclusion still holds.

**Last sweep: 2026-10-24** (rebase runbook Phase 5.5, the first under that
step).  Bugzilla REST, product `gcc`, summary containing `contract`: 57 open
in components `c++`/`libstdc++` (61 in all components; the extra four are
floating-point "contraction", the D front end, and PR109787 below), plus 3
resolved since 2026-09-05 (two "contraction"/RFE noise, and PR127228, a
libstdc++ hardening fix not about contract assertions).  Control:
PR125978 by id.  Measured against stock `17.0.0 20261025` (`cd7a85774a9`)
and the branch install of that day's rebase.  New and tracked: PR127911
(GCC-910) and PR128021 (GCC-911) in [`README.md`](README.md); PR126804 and
PR126881 below.  The next sweep starts from this date.

## Tracking bugs, not defects

| PR | Summary | Why not ours |
|----|---------|--------------|
| [119061](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119061) | [C++26] P2900R14 - Contracts | The umbrella bug for the feature itself. Nothing to reproduce. |
| [125834](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125834) | [C++29] P3097R3, Contracts for C++: virtual functions | Umbrella bug for the P3097 feature. We implement P3097 on this branch, but the PR tracks upstream's adoption, not a defect. |

## Out of scope for this branch

| PR | Summary | Why not ours |
|----|---------|--------------|
| [124088](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124088) | `-fcontracts-conservative-ipa` is unnecessary and wrong | A design argument about an IPA flag, not a bug with a reproducer. This branch has since removed the flag (refinable-body work, branch-history `0430-refinable-body-contracts`): a function holding a contract assertion is marked as a refinable body instead, which keeps the predicate available after the check inside the function -- what this PR asks for -- while callers that do not inline it stop using its facts.  Any comment on the PR is the user's to post. |
| [124264](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124264) | `__tu_has_violation` internal linkage breaks modules on PE-COFF | Needs a modules build on a PE-COFF target. Module integration is future work here -- P3595's per-module contract configuration is designed but not implemented -- and we do not target PE-COFF. The underlying observation -- that `declare_one_violation_handler_wrapper` gives the symbol internal linkage -- is worth remembering if module support is taken up. |
| [126572](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126572) | Default violation handler is a weak symbol and never resolves on PE-COFF | Platform-specific to x86_64-w64-mingw32; not reproducible on our Linux targets. Distinct from GCC-21, which was `-std=c++26` failing to link the handler at all. |
| [109787](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109787) | Warn about contract violations that can be detected at compile time (middle-end) | An enhancement request (2023, old `[[pre: ]]` syntax): warn when every path reaches the violation handler.  No defect to reproduce.  Neither compiler warns by default for its example; this branch's opt-in P4405 `diagnose` semantic answers a different question (an evaluation that cannot be proven, reported at the assertion).  Measured 2026-10-24. |

## Measured identical on both compilers

| PR | Summary | Measurement |
|----|---------|-------------|
| [125978](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125978) | Violation `comment()` overly verbose with a reference-returning `operator[]` | Ran the reporter's program on both: the violation text is **byte-identical**, including the fully-spelled `(((int)((const A*)(& foo))->A::operator[](0)) == 67)`. A quality-of-output question about how GCC prints a predicate, which this branch does not change. |

## No longer reproduces on stock trunk

These were real, and are gone -- fixed upstream, or by a change that reached
us both. The PRs are still open, which is why they appear in a search; the
behaviour is not.

| PR | Summary | Measurement |
|----|---------|-------------|
| [117436](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117436) | Using a data member inside a lambda from a contract fails | `struct f { int x; void f2() pre([this]{return x>0;}()) {} };` compiles clean on stock trunk and here. The report used the old `[[pre: ]]` attribute syntax. |
| [125537](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125537) | Crash on contracts with `auto` parameters | The reporter's `minimo(auto* const no) pre(no)` compiles clean on both; the reported ICE was `estimate_operator_cost, at tree-inline.cc:4433`. |
| [125645](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125645) | Packs in preconditions give garbage | `void fn(auto... args) pre(((args == 2) && ...))` called as `fn(2)` runs clean on both, with no spurious violation. |
| [125712](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125712) | ICE in `tsubst_contract_specifiers` on an out-of-line member template | The reporter's `Foo::func2` case compiles clean on both. Our branch also carries a fix in this area (`991099d97eb`), so the two may have converged. |
| [126050](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126050) | Infinite type-deduction loop on `operator<=>` with `auto` return | Compiled the reporter's own preprocessed attachment (saved locally as `126050_attachment.txt`): **both** compilers now give the same ordinary diagnostic, "use of `auto CustomNumber::operator<=>(const CustomNumber&) const` before deduction of `auto`". No loop, no hang. |
| [125011](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125011) | ICE in `tsubst` on a variable template used in a function template's postcondition | Reproduces on 16.1.0 and 16.2.0 (`internal compiler error: in tsubst, at cp/pt.cc:17246`/`17277`); the reporter's `square_v<x>` case and Comment 6's reduced `post(i: T{})` both ICE. Fixed upstream `58f3920e31f` (`rebuild_postconditions`: skip while `processing_template_decl`, do it properly at instantiation instead) -- landed 2026-08-28, absorbed here by the 2026-08-31 rebase. Clean on stock trunk and this branch. |
| [126804](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126804) | ICE when a pack of non-reference type is passed to a function taking a reference in a contract | Measured 2026-10-24: the reporter's explicit instantiation (`template void f(int)` with `pre(take(args...))`) and the postcondition form compile clean at `-O0` and `-O1` on stock `17.0.0 20261025` and here (the check is emitted: `take` is called).  16.1.0 and 16.2.0 still ICE (`visit_ref_for_mod_analysis`, ipa-prop.cc). |
| [126881](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126881) | ICE/wrong code calling a function whose precondition or postcondition folds over a parameter pack | Measured 2026-10-24: the explicit instantiations compile clean at `-O1`, and `f(42)` with `pre((... && args))` or `post((... && args))` runs clean at `-O0` and `-O1` on stock `17.0.0 20261025` and here; the controls `f(0)` report the violation on stock, so the check runs.  The `Args&&...` forms are PR126878 (GCC-22, rejected on stock, fixed here).  16.1.0 and 16.2.0 still ICE on the instantiation (`prepare_cmp_insn`, optabs.cc). |

## Method, so this can be repeated

The search is one request against Bugzilla's REST API, which is the
interface meant for programmatic queries:

```
curl "https://gcc.gnu.org/bugzilla/rest/bug?product=gcc&summary=contracts\
&resolution=---&include_fields=id,summary,status&limit=200"
```

`resolution=---` means unresolved. Comment text for a bug comes from
`rest/bug/<id>/comment`, and a whole bug including comments from
`show_bug.cgi?ctype=xml&id=1,2,3`, which takes a comma-separated list. Two
of these PRs supply only a preprocessed-source attachment; those are the
expensive ones, so download the attachment once and keep it rather than
re-fetching it for each measurement.
