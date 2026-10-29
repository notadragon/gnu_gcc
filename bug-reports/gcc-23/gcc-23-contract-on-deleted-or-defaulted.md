# GCC-23: [dcl.contract.func]/6 not applied to deleted or first-declaration-defaulted functions

**Status:** Fixed here (commit [9f09b82a8183](https://github.com/notadragon/gnu_gcc/commit/9f09b82a8183ceb7d1327ccfd6581281963229de))
**Resolved by:** `1090-contract-on-deleted-or-defaulted`
**Component:** c++ / contracts, declaration checking
**Also reported as:** [PR125403](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125403) -- the same defect reaching the
user as an ICE rather than a missing diagnostic: `Foo() post(empty()) =
default;` crashes stock trunk, where this branch reports "defaulted on its
first declaration cannot have a function-contract-specifier". Correlated
2026-09-05.

**Upstream Link:** [PR124486](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124486) (also PR125403, an ICE subsumed by the same fix)
**Affects:** stock g++ 16.2.0, trunk (17.0.0 20260901)

## Bug Report

[dcl.contract.func]/6 forbids a function-contract-specifier on a deleted
function, and on a function that is defaulted on its first declaration.
Before this fix, GCC silently accepted both:

```cpp
struct Del { void f (int x) pre (x > 0) = delete; };            // should error
struct DefCtor { DefCtor () pre (true) = default; };            // should error
```

Such a function has no body for the contract to guard, so the contract is
simply dropped -- the declaration looks accepted and the contract does
nothing, silently. Separately, a postcondition on an in-class defaulted
constructor (`DefCtorPost () post (true) = default;`) reached
`maybe_apply_function_contracts` and ICE'd there (PR125403), which
rejecting the declaration up front now prevents.

The paragraph's third case, a virtual function, is deliberately not
covered by this fix: P3097 lifts precisely that restriction, and this
branch implements P3097. `pr124486-accepted.C` pins that a contract on a
virtual function must keep working here, alongside the other shapes that
must remain accepted (a function defaulted somewhere other than its first
declaration, an ordinary function, and a deleted/defaulted function with
no contract at all).

## Reproducer

See [`gcc-23-contract-on-deleted-or-defaulted.C`](gcc-23-contract-on-deleted-or-defaulted.C)
(the diagnosed shapes) and
[`gcc-23-contract-on-deleted-or-defaulted-accepted.C`](gcc-23-contract-on-deleted-or-defaulted-accepted.C)
(the negative-space control: what must NOT be rejected) in this directory.

## Our Fix

`gcc/cp/contracts.cc`: new `check_contract_on_defaulted_or_deleted`,
declared in `gcc/cp/contracts.h`. Called from both the member path
(`grokfield` in `gcc/cp/decl2.cc`) and the namespace-scope path
(`cp_finish_decl` in `gcc/cp/decl.cc`); the defaulted check runs after
`DECL_INITIALIZED_IN_CLASS_P` is set, since
`DECL_DEFAULTED_IN_CLASS_P` reads it.

## Notes

The diagnosed-cases reproducer was combined from the fix commit's own
DejaGnu tests -- one shape per original file
(`pr124486-deleted.C`, `pr124486-ctor.C`, `pr124486-post.C`,
`pr124486-copy.C`, `pr124486-dtor.C`) -- found via
`git show 25bec7a0e6f --stat` and read with
`git show 25bec7a0e6f:gcc/testsuite/g++.dg/contracts/cpp26/<name>.C` for
each. DejaGnu directive lines were stripped, and the `{ dg-error ... }`
annotations were replaced with plain `// expected-error "..."` comments.
The fix commit's own log message explains why the originals are one case
per file rather than combined: DejaGnu intermittently stopped matching the
`= default` diagnostics when combined into one DejaGnu test, even though
the compiler emits each at exactly the expected line and "excess errors"
still passed -- "not root-caused, and not a property of the fix." That is
a DejaGnu harness quirk specific to its test-matching machinery, so it
does not apply to a plain standalone file compiled and read by eye;
combining the cases here is safe.

The accepted-control reproducer was extracted the same way from
`pr124486-accepted.C`. Its virtual-function case needs this branch's own
`-fcontracts-p3097` flag to compile without a diagnostic (P3097 is not
adopted, so stock GCC has no such flag); measured directly, both stock
g++ 16.2.0 and g++ trunk (17.0.0 20260901) reject that case with "contracts
cannot be added to virtual functions" under plain `-fcontracts`, which is
expected base-P2900 behavior and not part of this bug -- it is called out
in the file's own comments so it is not mistaken for a regression.

Measured directly against the stock binaries at
`gcc-16.2.0/bin/g++` and `gcc-trunk/bin/g++`
(17.0.0 20260901): both compile the diagnosed-cases file with exit code 0
and no diagnostic at all, confirming that a deleted function and four
different first-declaration-defaulted shapes with a contract are all
silently accepted (and the contract silently dropped) on both. Older
stock compilers (13.4.0, 14.4.0, 15.3.0) do not support C++26 contracts
syntax at all and are not applicable.

## Pending upstream comment (PR124486)

*Drafted for the user to post or drop; nothing here has been sent.  Checked
2026-09-25: no comment from berne@notadragon.com on PR124486 or PR125403.
Every claim re-measured 2026-09-25 against stock `17.0.0 20260922` (the
Compiler Explorer nightly) and 16.2.0.  PR124486 is assigned to Dinka
Ranns, so this offers the patch.  The last paragraph is the DejaGnu quirk
tracked as GCC-149; keep or drop it.  Attach
[9f09b82a8183](https://github.com/notadragon/gnu_gcc/commit/9f09b82a8183ceb7d1327ccfd6581281963229de) and its `pr124486-*.C` tests.
The virtual-function case of /6 is deliberately left out of it (this
branch implements P3097, which lifts it); stock already rejects contracts
on virtual functions, so nothing needs saying upstream about that.*

~~~text
In case it is useful, we have a patch for this: a
check_contract_on_defaulted_or_deleted, called from grokfield for members
and from cp_finish_decl at namespace scope, diagnosing a deleted function
and a function defaulted on its first declaration.  For the defaulted
case it has to run after DECL_INITIALIZED_IN_CLASS_P is set, because
DECL_DEFAULTED_IN_CLASS_P reads it.  Tests cover a deleted member, a
defaulted default constructor, copy constructor and destructor, a
postcondition on a defaulted constructor, and the shapes that must stay
accepted (defaulted other than on the first declaration, and deleted or
defaulted without a contract).

It also fixes PR125403: that ICE is a postcondition on an in-class
defaulted constructor, which /6 makes ill-formed, and rejecting the
declaration up front means it never reaches maybe_apply_function_contracts.

One testsuite oddity, not root-caused: with several of the = default
cases in one file, DejaGnu intermittently stopped matching their
diagnostics even though the compiler emitted each at the expected line
and "excess errors" passed.  One case per file was reliable.
~~~

## Pending upstream comment (PR125403)

*Same status and checks as the PR124486 comment above.*

~~~text
The program is ill-formed rather than valid: [dcl.contract.func]/6 says a
function defaulted on its first declaration shall not have a
function-contract-specifier-seq, and Foo() post(...) = default in the
class is exactly that.  So the fix is a diagnostic, which is PR124486.
The out-of-line "= default" workaround in the report is the well-formed
spelling.

On trunk (17.0.0 20260922) the ICE is now "in
maybe_apply_function_contracts, at cp/contracts.cc:1370", the assertion
that the body of a noexcept function is wrapped in a MUST_NOT_THROW_EXPR;
use_eh_spec_block does not wrap a defaulted function (!DECL_DEFAULTED_FN),
so it is the same assertion as PR127173.  16.2.0 segfaults instead.  With
the patch offered on PR124486 the testcase is rejected with "function
'Foo::Foo()' defaulted on its first declaration cannot have a
function-contract-specifier".
~~~
