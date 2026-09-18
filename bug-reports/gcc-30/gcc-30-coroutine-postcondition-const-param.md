# GCC-30: A coroutine's postcondition may odr-use a const by-value parameter

**Status:** Fixed here (commit [30fcf6303ac6](https://github.com/notadragon/gnu_gcc/commit/30fcf6303ac66a1d9a0c59fa0a5991dcf7522cea))
**Resolved by:** `1080-coroutine-postcondition-param`
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `accepts-invalid`
**Upstream Link:** [PR127296](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127296)
-- **FILED 2026-09-09**, UNCONFIRMED. (Searched 2026-09-05 before filing,
including resolved bugs: nothing matched.)
**Affects:** measured 2026-09-09 -- accepted on 16.1.0, 16.2.0 and both trunk
builds.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] a coroutine's postcondition may odr-use a const by-value parameter, which is ill-formed` |

Attachments:

| File | Description |
|---|---|
| [`coroutine-postcondition-const-param.cpp`](coroutine-postcondition-const-param.cpp) | The accepted const by-value parameter, with four controls |
| `coroutine-postcondition-const-param.ii` | Preprocessed source, produced with `-save-temps` |

This is the one reproducer here that needs an `#include` (`<coroutine>`), so
unlike the others it is not its own preprocessed source.

````
An odr-use of a non-reference parameter in a coroutine's postcondition is
ill-formed.  GCC accepts it whenever the parameter is written const.

[dcl.fct.def.coroutine] says the consequence outright, in a note:

  "An odr-use of a non-reference parameter in a postcondition assertion of a
   coroutine is ill-formed."

The two rules that produce it are [dcl.contract.func], which requires a
parameter a postcondition odr-uses to be const, and
[dcl.fct.def.coroutine]/5, which says the coroutine's definition behaves
as if its parameters have no top-level cv-qualifiers.  Not having
cv qualifiers makes it impossible for a coroutine definition's parameters
to be const, and thus they are invalid if they are non-reference parameters
used from a postcondition.

```
#include <coroutine>

struct Task {
    struct promise_type {
        Task get_return_object() { return {}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() { }
        void unhandled_exception() { }
    };
};

// THE BUG: accepted; ill-formed.
Task by_value(const int x) post(x > 0) { co_return; }

// CONTROL: the non-const spelling is caught, by the ordinary const rule
// rather than by anything coroutine-aware.  That is what says the coroutine
// restriction itself is missing rather than merely mis-worded.
Task non_const(int x) post(x > 0) { co_return; }

// CONTROL: a reference parameter is fine -- [dcl.fct.def.coroutine]/5 binds
// its frame copy to the same object -- and must keep compiling.
Task by_ref(const int& x) post(x > 0) { co_return; }

// CONTROL: a precondition may name a by-value parameter; the const rule, and
// so this restriction, are postcondition rules.
Task in_pre(int x) pre(x > 0) { co_return; }

// CONTROL: a non-coroutine with the same signature is unaffected.
int not_a_coroutine(const int x) post(x > 0) { return x; }
```

```
$ ./gcc-16.2.0/bin/g++ -std=c++26 -fsyntax-only \
    coroutine-postcondition-const-param.cpp
coroutine-postcondition-const-param.cpp:32:28: error: a value parameter used in a postcondition must be const
   32 | Task non_const(int x) post(x > 0) { co_return; }
      |                            ^
coroutine-postcondition-const-param.cpp:32:20: note: parameter declared here
   32 | Task non_const(int x) post(x > 0) { co_return; }
      |                ~~~~^
```

Only non_const is diagnosed, and that is the ordinary const rule firing, not
anything coroutine-aware.  by_value -- the same function with the parameter
written const to satisfy that rule -- is accepted silently.

  spelling                                        result
  Task f(int x) post(x > 0)                       rejected (const rule)
  Task f(const int x) post(x > 0)                 accepted, should not be

So the coroutine restriction is missing outright rather than mis-worded, and
the const rule was masking its absence: every spelling a user would try
first happens to be caught by the other rule.

DISCOVERY

Found by an audit comparing every test in a C++26 contracts implementation's
own suite against stock trunk, looking for cases that pass locally only
because they were fixed locally.

ANALYSIS

The const rule and the coroutine rule are checked independently, and only
the first is implemented.  Because the const rule rejects the non-const
spelling, the coroutine case is only reachable through the const one, which
nothing examines -- so the gap is invisible to any test that does not
deliberately write the parameter const.

VERSIONS -- all on x86_64-linux-gnu

  source              version                       accepts by_value
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260909, 919c0d16c91  yes
  local build -g      17.0.0 20260909, 7dab38c9d71  yes

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

See [`coroutine-postcondition-const-param.cpp`](coroutine-postcondition-const-param.cpp)
in this directory, with four controls: the non-const spelling, a reference
parameter (fine -- its frame copy binds to the same object), a precondition
(the const rule and so this restriction are postcondition rules), and a
non-coroutine with the same signature.

## Our Fix

Diagnose it once the function is known to be a coroutine, which is only after
its body has been parsed -- and therefore after its contracts. The parameters
carry the "odr-used in a postcondition" flag set when the predicate was
walked, so the check is a walk over them at that later point.

Test: `gcc/testsuite/g++.dg/contracts/cpp26/coroutine-postcondition-param.C`.

## Notes

Found by a deliberate contracts-x-coroutines sweep; surfaced as an untracked
upstream bug by the 2026-09-05 audit comparing every plain-`-fcontracts` test
against stock trunk.

Clang had the same gap and was fixed alongside; contracts are not upstream in
Clang, so there is nothing to file there.
