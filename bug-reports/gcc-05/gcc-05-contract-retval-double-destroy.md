# GCC-5: Contract on a function returning a class with a non-trivial destructor double-destroys the return value

**Status:** Fixed here (commit `6e5a47c2a39`)
**Resolved by:** `1240-retval-cleanup-not-respliced`
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `wrong-code`, `ice-on-valid-code`
**Upstream Link:** [PR127281](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127281) -- **FILED 2026-09-09**, UNCONFIRMED. (Searched
2026-09-05 before filing, including resolved bugs, on comment text
`contract return value destroyed twice`: nothing matched.)
**Affects:** measured 2026-09-09 -- both symptoms reproduce on every
published GCC 16 (`16.1.0` and `16.2.0`) and on trunk `17.0.0 20260909
(experimental)`. 15.3.0 and 14.4.0 reject the `pre` syntax and 13.4.0 has no
`-std=c++26` at all, so this affects every release that has ever accepted the
syntax. Both are fixed on this branch (re-confirmed against the 2026-09-09
build).

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] a contract emits the return-value cleanup twice: returned object destroyed twice, and an ICE in gimple_add_tmp_var` |

Attachments:

| File | Description |
|---|---|
| [`double-destroy.cpp`](double-destroy.cpp) | Returned object destroyed twice: exits 255, and 0 with the contract deleted |
| [`ice.cpp`](ice.cpp) | The same defect as an ICE in gimple_add_tmp_var, with the necessary ingredients listed |

Neither has any `#include`, so each is already its own preprocessed source.

````
A contract specifier on a function returning a class with a non-trivial
destructor makes the compiler emit the machinery that destroys the return
object twice.  Depending on the shape of the function that either
double-destroys a returned object at run time, in a program that compiles
without a diagnostic, or ICEs.


1. WRONG CODE -- attachment double-destroy.cpp

```
int live = 0;

struct Counted {
    int v;
    Counted(int x) : v(x) { ++live; }
    Counted(const Counted& o) : v(o.v) { ++live; }
    ~Counted() { --live; }
};

struct ThrowOnDestroy {
    bool armed;
    ~ThrowOnDestroy() noexcept(false) { if (armed) throw 42; }
};

Counted f(bool arm) pre(true)   // delete `pre(true)' and the bug goes away
{
    ThrowOnDestroy guard{arm};
    Counted result(7);
    return result;              // built, then `guard' throws on the way out
}

int main()
{
    try { f(true); } catch (int) { }
    return live;                // 0 expected; -1 (exit 255) as it stands
}
```

```
$ ./gcc-16.2.0/bin/g++ -std=c++26 double-destroy.cpp -lstdc++exp -o dd
$ ./dd; echo $?
255
```

255 is live == -1: one construction, two destructions.  Deleting the
pre(true) -- and changing nothing else -- gives 0.  The predicate's content
is irrelevant; it is never false and never fails.  post(true) behaves the
same way.


2. ICE -- attachment ice.cpp

```
struct D { ~D(); };

D f(int n) pre(true)
{
    D r;
    if (n)
        return r;
    int other;
    return r;
}
```

```
$ ./gcc-16.2.0/bin/g++ -std=c++26 -c ice.cpp
ice.cpp: In function 'D f(int)':
ice.cpp:22:3: internal compiler error: in gimple_add_tmp_var, at gimplify.cc:841
   22 | D f(int n) pre(true)
      |   ^
0x79134f22a1c9 __libc_start_call_main
	../sysdeps/nptl/libc_start_call_main.h:58
0x79134f22a28a __libc_start_main_impl
	../csu/libc-start.c:360
```

(Line 22 is the definition of f in the attachment, which carries a comment
header.)  -fsyntax-only alone compiles clean; the function has to be
emitted.

From a trunk build (7dab38c9d71) configured the same way, but with -g, we
get a more complete stack trace:

```
0x24e67c5 internal_error(char const*, ...)
	gcc/diagnostic-global-context.cc:787
0x824eb1 fancy_abort(char const*, int, char const*)
	gcc/diagnostics/context.cc:1813
0x7cfa80 gimple_add_tmp_var(tree_node*)
	gcc/gimplify.cc:845
0xd9ab1b gimplify_decl_expr
	gcc/gimplify.cc:2075
0xd9404f gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
	gcc/gimplify.cc:20713
0xd95da6 gimplify_stmt(tree_node**, gimple**)
	gcc/gimplify.cc:8577
0xd93acb gimplify_statement_list
	gcc/gimplify.cc:2166
0xd93acb gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
	gcc/gimplify.cc:20967
0xd95da6 gimplify_stmt(tree_node**, gimple**)
	gcc/gimplify.cc:8577
0xd97823 gimplify_body(tree_node*, bool)
	gcc/gimplify.cc:21823
0xd97c6f gimplify_function_tree(tree_node*)
	gcc/gimplify.cc:22032
0xbbbd67 cgraph_node::analyze()
	gcc/cgraphunit.cc:691
0xbbe7ef analyze_functions
	gcc/cgraphunit.cc:1270
0xbbf372 symbol_table::finalize_compilation_unit()
	gcc/cgraphunit.cc:2593
```

Every ingredient is necessary -- dropping any one of these compiles clean:

  - the contract specifier (pre or post, predicate irrelevant);
  - a return type with a non-trivial destructor;
  - returning a *named* local.  Returning a prvalue, or a call result, is
    clean;
  - a declaration of any kind after the first return statement.  The
    "int other;" suffices, it need not have a destructor, and moving it
    above the if is clean.


DISCOVERY

Found when migrating uses of BSLS_ASSERT to pre in the BDE libraries, in a
date/calendar component whose members return a class type by value.  One
translation unit ICEd.  The ten-line case above is what was left after
reducing that function against the real component and then dropping the
library entirely -- the original had two loops, which turned out to be
incidental.  The wrong-code case came out of checking that reduction: a shape
that does not happen to trip the gimplifier's assertion just runs the
duplicated cleanup, silently.


ANALYSIS

The two symptoms are one defect.  maybe_apply_function_contracts runs from
finish_function with the sk_function_parms level current, and wraps the
already-finished body in an artificial block.  do_poplevel
(gcc/cp/semantics.cc) pops that block's own level before calling
maybe_splice_retval_cleanup (gcc/cp/except.cc), so the latter sees
sk_function_parms a second time -- the very test it uses to recognise a
function body, and one the real body's closing brace has already passed.
current_retval_sentinel therefore gets a second DECL_EXPR, and, where
cp_function_chain->throwing_cleanup is set, a second retval CLEANUP_STMT
carrying the same destructor.  Case 1 has the throwing cleanup and so runs
the duplicated destructor; case 2 has the duplicated sentinel declaration,
which trips the assertion in gimple_add_tmp_var.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       ICE   wrong code
  compiler-explorer   16.1.0                        yes   yes
  compiler-explorer   16.2.0                        yes   yes
  compiler-explorer   17.0.0 20260909, 919c0d16c91  yes   yes
  local build -g      17.0.0 20260909, 7dab38c9d71  yes   yes

On trunk the assertion is at gimplify.cc:845 rather than :841.

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

The two attachments above,
[`double-destroy.cpp`](double-destroy.cpp) and
[`ice.cpp`](ice.cpp), reduced and free of internal annotation.

Superseded, retained only until this entry is filed:
[`gcc-05-calendar-gimplify-ice.cpp`](gcc-05-calendar-gimplify-ice.cpp), the
original 50-line `Calendar` case from the BDE translation unit that found it
-- its two loops turn out to be incidental, and `ice.cpp` reduces the
same ICE to ten lines -- and
[`gcc-05b-contract-retval-double-destroy.cpp`](gcc-05b-contract-retval-double-destroy.cpp),
from which the attachment differs only in its comments.

## Our Fix

`gcc/cp/contracts.cc`: hide `current_retval_sentinel` across the artificial
contract-check block (via `make_temp_override`) so the splice logic takes
its already-spliced early exit instead of re-emitting the cleanup.

Deliberately **not** offered upstream as a patch: per GCC policy the fix
needs to be reimplemented by a human contributor. The report above stops at
the diagnosis.

## Notes

Filed 2026-09-09 as [PR127281](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127281), UNCONFIRMED.

A maintainer added the `c++-contracts` keyword four minutes after filing,
unprompted -- the enter-bug form has no Keywords field, so that is how they
arrive. The behavioural keywords did not follow: this is both a `wrong-code`
and an `ice-on-valid-code` bug and the PR is tagged as neither, which are
exactly the tags triage queries run on. Worth adding by editing the bug.
See "Keywords: not ours to set" in [`../README.md`](../README.md).
