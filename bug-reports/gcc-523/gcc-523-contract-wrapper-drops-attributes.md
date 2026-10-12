# GCC-523: Contract wrappers drop the callee's `noreturn` and `nodiscard`, and diagnostics name the wrapper

**Status:** Fixed here (commit [7017fb358e8f](https://github.com/notadragon/gnu_gcc/commit/7017fb358e8f65f0a84cfce8de3a66220ef6b014)); the P3097 virtual-function half is
branch-only
**Resolved by:** `2400-p3595`
**Component:** c++
**Keywords (ours -- upstream sets its own):** `diagnostic`
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST, summary
and comment text, on `contract_wrapper`, `client-check`, `contract
nodiscard`, `contract noreturn`, plus every open bug with `contract` in its
summary; the only `client-check` hit is PR124089 (flag naming).  Earlier
search 2026-09-25, same result)
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0 and stock trunk
`17.0.0 20260922` (`f008f03eff8`) with `-fcontracts-client-check=all`
(`-O2` is not needed); this branch with a P3595 caller-side configuration,
and with `-fcontracts-p3097` for a `[[nodiscard]]` contracted virtual
function under the default configuration
**Clang:** caller-side checking is not implemented; the P3097 case is
correct (Clang warns "ignoring return value of function declared with
'nodiscard' attribute")
**Found:** 2026-09-25, review round #12 fan-out (g-a-8)

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] -fcontracts-client-check wrappers lose noreturn and nodiscard, and warnings name the wrapper` |

Attachments:

| File | Description |
|---|---|
| [`contract-wrapper-drops-attributes.cpp`](contract-wrapper-drops-attributes.cpp) | A `noreturn`, a `nodiscard` and a `warn_unused_result` callee with preconditions |

````
With -fcontracts-client-check=all, calls to a function with contracts go
through a generated wrapper that does not carry the callee's attributes, so
diagnostics that depend on them change.

```
[[noreturn]] void die (int x) pre (x > 0);
int f (int x) { if (x) return x; die (x); }
[[nodiscard]] int nd (int x) pre (x > 0);
void use1 () { nd (1); }
__attribute__((warn_unused_result)) int wur (int x) pre (x > 0);
void use2 () { wur (1); }
```

```
$ g++ -std=c++26 -fcontracts -fcontracts-client-check=all -Wall -c \
    contract-wrapper-drops-attributes.cpp
contract-wrapper-drops-attributes.cpp: In function 'int f(int)':
contract-wrapper-drops-attributes.cpp:11:38: warning: control reaches end of non-void function [-Wreturn-type]
   11 | int f (int x) { if (x) return x; die (x); }
      |                                  ~~~~^~~
contract-wrapper-drops-attributes.cpp: In function 'void use2()':
contract-wrapper-drops-attributes.cpp:15:20: warning: ignoring return value of 'int wur.contract_wrapper(int)' declared with attribute 'warn_unused_result' [-Wunused-result]
   15 | void use2 () { wur (1); }
      |                ~~~~^~~
```

Without -fcontracts-client-check=all the same command gives what is
expected: no -Wreturn-type, since die is noreturn; a warning for nd (1);
and the wur (1) warning naming the function the user called:

```
contract-wrapper-drops-attributes.cpp: In function 'void use1()':
contract-wrapper-drops-attributes.cpp:13:19: warning: ignoring return value of 'int nd(int)', declared with attribute 'nodiscard' [-Wunused-result]
   13 | void use1 () { nd (1); }
      |                ~~~^~~
contract-wrapper-drops-attributes.cpp:12:19: note: declared here
   12 | [[nodiscard]] int nd (int x) pre (x > 0);
      |                   ^~
contract-wrapper-drops-attributes.cpp: In function 'void use2()':
contract-wrapper-drops-attributes.cpp:15:20: warning: ignoring return value of 'int wur(int)' declared with attribute 'warn_unused_result' [-Wunused-result]
   15 | void use2 () { wur (1); }
      |                ~~~~^~~
```

Expected: the same diagnostics with and without the option.  The
noreturn loss is not only a diagnostic matter: the -Wreturn-type above is
flow analysis reporting that the call through the wrapper is no longer
known not to return.


DISCOVERY

Found while reviewing an implementation of C++26 contracts for how
caller-side checking interacts with function attributes.


ANALYSIS

From reading the code, not confirmed in a debugger:
build_contract_wrapper_function (gcc/cp/contracts.cc) copies neither
TREE_THIS_VOLATILE nor DECL_ATTRIBUTES (nor TREE_NOTHROW) from the callee,
and diagnostics about the call see the wrapper's decl.  Copying them, and
having call-site diagnostics use get_orig_func_for_wrapper, would restore
both.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       attributes lost
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260922, f008f03eff8  yes
````

## Our Fix

`gcc/cp/contracts.cc`, `build_contract_wrapper_function`: the wrapper copies
the callee's `TREE_THIS_VOLATILE` (noreturn), and
`define_one_contract_wrapper_func` builds its body without it, so the
wrapper's own `return callee (...)` is not reported.  `warn_unused_result` is
a type attribute, which the wrapper already shares.  `maybe_warn_nodiscard`
(`gcc/cp/cvt.cc`) looks through a wrapper to the callee, and the C++
pretty-printer (`dump_function_decl`, `gcc/cp/error.cc`) names a wrapper as
the function it wraps, so every diagnostic about the call names the callee
(`get_orig_func_for_wrapper` is now exported).  Regression tests:
`g++.dg/contracts/cpp26/caller-wrapper-attributes.C`,
`virtual-wrapper-nodiscard.C`.

## Notes (ours, not for the report)

On the branch, `-fcontracts-client-check=all` no longer wraps; the same
symptoms appear with a P3595 caller-side configuration (caller observe,
callee ignore).  The branch's P3097 interface wrapper for a virtual function
is built by the same function, so `struct B { [[nodiscard]] virtual int v
(int x) pre (x > 0); }; void u (B& b) { b.v (1); }` loses its
-Wunused-result under `-fcontracts-p3097` with the default configuration.

Watch tests: `g++.dg/contracts/cpp26/caller-wrapper-attributes.C`
(caller-side configuration; per-line xfail) and
`virtual-wrapper-nodiscard.C` (P3097; xfail dg-warning).
Clang mirror (expected pass, P3097 only; Clang has no caller-side checking):
`clang/test/Contracts/virtual-wrapper-drops-nodiscard.cpp`.
