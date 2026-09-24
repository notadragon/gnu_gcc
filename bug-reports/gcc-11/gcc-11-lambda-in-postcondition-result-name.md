# GCC-11: Lambda in a postcondition with a result-name-introducer fails to parse

**Status:** Fixed here (commit [2c3b9f330234](https://github.com/notadragon/gnu_gcc/commit/2c3b9f330234300918db6338363eee65c90b72ab))
**Resolved by:** `1160-lambda-in-postcondition-result-name`
**Component:** c++ / parser
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** [PR127284](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127284)
-- **FILED 2026-09-09**, UNCONFIRMED. (Searched 2026-09-05 and again
2026-09-09 before filing, across summaries for `lambda postcondition`,
`postcondition result name`, `contract lambda parse`, `lambda contract`,
`result-name`: nine distinct hits, none this bug -- PR124648 is an ICE on a
*pre*condition, PR117431/435/436 are captures.) **PR125537 is the sibling**,
not a duplicate -- its fix narrowed the other of the two raise sites; see the
report.
**Affects:** measured 2026-09-09 -- the free-function case is rejected on
16.1.0, 16.2.0 and trunk. The member-function case, which earlier notes here
recorded as accepted, is in fact rejected on 16.x and accepted only on trunk:
upstream has fixed that half and not this one.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] a lambda in a non-member function's postcondition fails to parse when there is a result-name-introducer` |

Attachments:

| File | Description |
|---|---|
| [`lambda-in-postcondition-result-name.cpp`](lambda-in-postcondition-result-name.cpp) | Parse failure on the free-function form, with the accepted controls beside it |

````
A lambda-expression in the predicate of a non-member function's
postcondition does not parse when the postcondition has a
result-name-introducer.  

```
bool ok() { return true; }

// (1) THE BUG: free function, postcondition, result-name-introducer.
int broken(int x) post(r : [] { return ok(); }()) { return x; }

// (2) Control: free function, postcondition, NO result name.  Accepted.
int no_result_name(int x) post([] { return ok(); }()) { return x; }

// (3) Control: free function, precondition.  Accepted.
int precondition(int x) pre([] { return ok(); }()) { return x; }

// (4) Control: member function, postcondition, result-name-introducer.
//     Version-dependent, which is why it is commented out: 16.1.0 and
//     16.2.0 reject this too, with a DIFFERENT diagnostic ("expected
//     conditional-expression"), while trunk accepts it.  The member-function
//     half was fixed by PR c++/125537 (r17-3710, commit 943089ca86f), which
//     narrowed the same raise at the other of its two sites; the
//     free-function half, row (1), was not.  Uncomment to see whichever
//     your compiler does.
//
// struct S {
//     int mem(int x) post(r : [] { return ok(); }()) { return x; }
// };

int main() { }
```

```
lambda-in-postcondition-result-name.cpp:22:30: error: expected ')' before '{' token
   22 | int broken(int x) post(r : [] { return ok(); }()) { return x; }
      |                       ~      ^~
      |                              )
lambda-in-postcondition-result-name.cpp:22:48: error: expected unqualified-id before ')' token
   22 | int broken(int x) post(r : [] { return ok(); }()) { return x; }
      |                                                ^
```

CONTROLS

Neither postconditions nor lambdas in predicates are broken on their own; it
takes the combination.  Measured 2026-09-09:

  case                                          16.2.0        trunk
  free fn, post(r : []{...}()), result name     REJECTED      REJECTED
  free fn, post([]{...}()), no result name      accepted      accepted
  free fn, pre([]{...}())                       accepted      accepted
  member fn, post(r : []{...}()), result name   REJECTED      accepted

The last row is worth stating plainly: on 16.x the member-function form is
also rejected, with a DIFFERENT diagnostic ("expected
conditional-expression"), and trunk accepts it.  So the member half is
already fixed upstream and the free-function half is not.  A member's
contract is deferred and late-parsed once the class is complete, whereas a
free function's is parsed straight off the declarator, which is why the two
halves can diverge at all.

DISCOVERY

Found while implementing capture of enclosing entities by a lambda written
inside a contract predicate ([expr.prim.lambda.capture]/3.3, PR117435).  It
surfaced as a spurious failure in that work's own regression test and was
isolated out of it; no capture is needed to provoke it.

ANALYSIS

The non-deferred contract path raises processing_template_decl whenever a
result-name-introducer is present, because the result variable is typed with
make_auto () -- the return type is not available while the predicate is
parsed off the declarator -- so the predicate is treated as dependent.
Lambda parsing does not cope with that flag being raised outside a real
template; that is PR99546, and cp_parser_lambda_expression already carries a
workaround for it, gated on current_binding_level->requires_expression.

That accounts for every row above: postconditions only, because only they
have a result name; non-members only, because a member's late-parsed path
raises the flag only for an undeduced return type; and the lambda's position
within the predicate is irrelevant, the flag being global for the parse.

That asymmetry is recent and traceable.  r17-3710, commit 943089ca86f,
"c++/contracts: unify condition for pseudo-template mode" [PR c++/125537],
made the raise in cp_parser_late_contract_condition -- the member path --
conditional on the result binding having an undeduced return type, which is
why the member row changed between 16.2.0 and trunk.  The non-deferred site
in cp_parser_function_contract_specifier still raises whenever a result
identifier is present, so the free-function row did not change.  This is the
same defect at the site that fix did not reach: not a regression from it,
and not a duplicate of 125537, which described a different symptom.

Extending that gate to the contract scope is half of a fix.  The other half
is needed because the two constructs differ in a way the workaround's own
comment does not mention: a requires-expression's operand is never
substituted afterwards, so a non-templatey lambda built there is never
looked at again, whereas a contract predicate on this path IS substituted --
by rebuild_postconditions, once the return type is known.  tsubst_lambda_expr
then asks tsubst_function_decl to substitute into an operator() with no
template info, which is what that function opens by asserting it is never
asked to do.

VERSIONS -- all on x86_64-linux-gnu

  source              version                       free fn w/ result name
  compiler-explorer   16.1.0                        rejected
  compiler-explorer   16.2.0                        rejected
  compiler-explorer   17.0.0 20260909, 919c0d16c91  rejected
  local build -g      17.0.0 20260909, 7dab38c9d71  rejected

```
$ ./gcc-16.2.0/bin/g++ -v
Using built-in specs.
COLLECT_GCC=./gcc-16.2.0/bin/g++
COLLECT_LTO_WRAPPER=.../gcc-16.2.0/bin/../libexec/gcc/x86_64-linux-gnu/16.2.0/lto-wrapper
Target: x86_64-linux-gnu
Configured with: ../gcc-16.2.0/configure --prefix=/opt/compiler-explorer/gcc-build/staging --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=x86_64-linux-gnu --disable-bootstrap --enable-multiarch --with-abi=m64 --with-multilib-list=m32,m64,mx32 --enable-multilib --enable-clocale=gnu --enable-languages=c,c++,fortran,ada,objc,obj-c++,go,d,m2,rust,cobol,algol68 --enable-ld=yes --enable-gold=yes --enable-libstdcxx-time=yes --enable-linker-build-id --enable-lto --enable-plugins --enable-threads=posix --with-pkgversion=Compiler-Explorer-Build-gcc--binutils-2.44
Thread model: posix
Supported LTO compression algorithms: zlib
gcc version 16.2.0 (Compiler-Explorer-Build-gcc--binutils-2.44)
```
````

## Reproducer

See [`lambda-in-postcondition-result-name.cpp`](lambda-in-postcondition-result-name.cpp)
and [`gcc-11-lambda-in-postcondition-testcases.C`](gcc-11-lambda-in-postcondition-testcases.C)
in this directory.

## Our Fix

`gcc/cp/parser.cc` and `gcc/cp/pt.cc`: extend the existing PR99546
lambda-parsing workaround to contract scope, and make `tsubst_lambda_expr`
a no-op for a lambda whose `operator()` already carries no template info.

## Notes

Long-standing since `post` was first accepted.
