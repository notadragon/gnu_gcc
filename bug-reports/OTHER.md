# Open Upstream Contracts PRs That Are Not Ours

The correlation sweep of 2026-09-05 walked every open PR in GCC Bugzilla with
"contracts" in the summary -- 33 of them -- and measured each against stock
g++ trunk (17.0.0 20260901) and this branch. The ones that turned out to
correlate with our work are rows in [`README.md`](README.md). **This file
records the rest, so the same PRs are not investigated again.**

A PR listed here can still become relevant: "not ours" is a measurement, and
measurements go stale. Each entry says what was measured, so a future sweep
can tell whether the conclusion still holds.

## Tracking bugs, not defects

| PR | Summary | Why not ours |
|----|---------|--------------|
| [119061](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119061) | [C++26] P2900R14 - Contracts | The umbrella bug for the feature itself. Nothing to reproduce. |
| [125834](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125834) | [C++29] P3097R3, Contracts for C++: virtual functions | Umbrella bug for the P3097 feature. We implement P3097 on this branch, but the PR tracks upstream's adoption, not a defect. |

## Out of scope for this branch

| PR | Summary | Why not ours |
|----|---------|--------------|
| [124088](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124088) | `-fcontracts-conservative-ipa` is unnecessary and wrong | A design argument about an IPA flag, not a bug with a reproducer we can fix. It does overlap the deferred IPA/optimisation item in `p3850impl/PENDING.md`, so if that work is ever taken up, read this PR first. |
| [124264](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124264) | `__tu_has_violation` internal linkage breaks modules on PE-COFF | Needs a modules build on a PE-COFF target. Module integration is future work here (see `p3850impl/p3595-*`), and we do not target PE-COFF. The underlying observation -- that `declare_one_violation_handler_wrapper` gives the symbol internal linkage -- is worth remembering if module support is taken up. |
| [126572](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126572) | Default violation handler is a weak symbol and never resolves on PE-COFF | Platform-specific to x86_64-w64-mingw32; not reproducible on our Linux targets. Distinct from GCC-21, which was `-std=c++26` failing to link the handler at all. |

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

## Method, so this can be repeated

The search is one request -- the Anubis gate covers Bugzilla's HTML UI, not
its REST API:

```
curl "https://gcc.gnu.org/bugzilla/rest/bug?product=gcc&summary=contracts\
&resolution=---&include_fields=id,summary,status&limit=200"
```

`resolution=---` means unresolved. Comment text for a bug comes from
`rest/bug/<id>/comment`, and a whole bug including comments from
`show_bug.cgi?ctype=xml&id=1,2,3`, which takes a comma-separated list. Two
of these PRs supply only a preprocessed-source attachment; those are the
expensive ones, and for `126050` a copy was already saved under
`p3850impl/gcc-bugs/bugzilla/`.
