# GCC-13: A lambda with a contract specifier that names its own parameter ICEs on template instantiation

**Status:** Fixed here (commit `5852998dd08`)
**Resolved by:** `4640-contract-substitution-plumbing`
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `ice-on-valid-code`
**Upstream Link:** [PR127287](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127287)
-- **FILED 2026-09-09**, UNCONFIRMED. (Searched 2026-09-05 before filing,
including resolved bugs: nothing matched.)
**Affects:** measured 2026-09-09 -- ICEs on 16.1.0, 16.2.0 and both trunk
builds. A second face, a postcondition with a result name instantiated
twice, ICEs only on a **checking** build; see the report.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] ICE in expand_expr_real_1 when a lambda's own contract specifier names its parameter and the lambda is instantiated from a template` |

Attachments:

| File | Description |
|---|---|
| [`contract-lambda-in-template-ice.cpp`](contract-lambda-in-template-ice.cpp) | ICE in expand_expr_real_1, with the three necessary ingredients listed |

````
A lambda with a contract specifier whose predicate names the lambda's
own parameter ICEs when the lambda is instantiated as part of a template.

```
template <class T>
int ft(T a)
{
    auto inner = [](int b) pre(b > 0) { return b; };
    return inner(a);
}

int f() { return ft(1); }
```

```
$ ./gcc-16.2.0/bin/g++ -std=c++26 -c \
    contract-lambda-in-template-ice.cpp
during RTL pass: expand
contract-lambda-in-template-ice.cpp: In lambda function:
contract-lambda-in-template-ice.cpp:15:18: internal compiler error: in expand_expr_real_1, at expr.cc:11648
   15 |     auto inner = [](int b) pre(b > 0) { return b; };
      |                  ^
0x74ee6602a1c9 __libc_start_call_main
	../sysdeps/nptl/libc_start_call_main.h:58
0x74ee6602a28a __libc_start_main_impl
	../csu/libc-start.c:360
```

From a trunk build (7dab38c9d71) configured the same way, but with -g, we
get a more complete stack trace:

```
0x24e67c5 internal_error(char const*, ...)
	gcc/diagnostic-global-context.cc:787
0x824eb1 fancy_abort(char const*, int, char const*)
	gcc/diagnostics/context.cc:1813
0x7ccda4 expand_expr_real_1(tree_node*, rtx_def*, machine_mode, expand_modifier, rtx_def**, bool)
	gcc/expr.cc:11783
0xbed6c0 expand_normal(tree_node*)
	gcc/expr.h:329
0xbed6c0 do_compare_and_jump
	gcc/dojump.cc:1288
0xbee643 do_jump_1
	gcc/dojump.cc:242
0xb7b4f8 expand_gimple_cond
	gcc/cfgexpand.cc:3054
0xb7b4f8 expand_gimple_basic_block
	gcc/cfgexpand.cc:6395
0xb7cb7f execute
	gcc/cfgexpand.cc:7295
```

Each ingredient is necessary -- dropping any one compiles clean:

  - the contract is a specifier on the lambda.  A contract_assert in the
    lambda's body is part of the body, substitutes with it, and is fine;
  - the predicate names the lambda's own parameter.  pre(true), or a
    predicate naming only a global, is fine;
  - the lambda is instantiated as part of a template.  The same lambda in a
    non-template function is fine, and so is one that is never called.

Neither nesting nor a generic lambda is required, though this was first
found inside one.

The same defect reached through a postcondition rather than a precondition
surfaces elsewhere, and only where tree checking is enabled:

```
template <class T> int ft(T a) {
    auto inner = [](int b) post(r : r > 0) { return b; };
    return inner((int)a);
}
int f() { return ft(1) + ft(2.0); }
```

That is accepted by 16.1.0 and 16.2.0 and by a trunk build configured
--enable-checking=release, and ICEs on a trunk build taking configure's
development default of --enable-checking=yes,extra.  Two instantiations are
needed: the first silently reparents the pattern's result variable onto
itself, and the second trips over it.  Though this hits an assertion
in checked builds, we were not able to find a case where the generated
code was corrupted by the inconsistent trees.

DISCOVERY

Found while implementing capture of enclosing entities by a lambda written
inside a contract predicate ([expr.prim.lambda.capture]/3.3, PR117435).  The
matrix of shapes was first classified on a release build, where the second
face is compiled out -- a checking build contradicted the recorded table
within a minute of first being used.

ANALYSIS

tsubst_function_decl copies a function's contract specifiers onto the
instantiation without substituting them -- its own comment says so -- because
for an ordinary function regenerate_decl_from_template substitutes them
later.  A lambda's operator() never reaches regenerate_decl_from_template:
tsubst_lambda_expr builds it and substitutes its body on the spot.

So the instantiation keeps the pattern's contract trees, whose predicate
names the pattern's PARM_DECLs and whose result binding belongs to the
pattern.  Nothing detects this, so it surfaces wherever the stale tree is
first used -- which is why it looks like several unrelated bugs depending on
the shape and on the build's checking level.

VERSIONS -- all on x86_64-linux-gnu

  source              version                       ICE
  compiler-explorer   16.1.0                        yes (expr.cc:11648)
  compiler-explorer   16.2.0                        yes (expr.cc:11648)
  compiler-explorer   17.0.0 20260909, 919c0d16c91  yes (expr.cc:11783)
  local build -g      17.0.0 20260909, 7dab38c9d71  yes (expr.cc:11789)

The compiler-explorer trunk build takes configure's development default,
--enable-checking=yes,extra; the 16.x builds and the local build are
--enable-checking=release.  That difference is what the second face above
depends on.

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

See [`contract-lambda-in-template-ice.cpp`](contract-lambda-in-template-ice.cpp)
in this directory.

## Our Fix

`gcc/cp/pt.cc`: substitute the lambda's own contract specifiers in
`tsubst_lambda_expr` against the pattern, before the body, via a new shared
`subst_contract_specifiers` helper.

## Notes

Four distinct crash sites were observed depending on build and instantiation
count; the reproducer is written to exercise more than one of them.

A non-checking build was checked for wrong *values*, not just for whether it
crashes: a probe designed to catch a stale result binding reading the wrong
object across instantiations (value-correctness assertions, and a
non-trivial-destructor stress test modelled on GCC-5's) found no
misbehaviour on stock 16.1.0, 16.2.0, or trunk-release, across several
instantiation orders and counts and at `-O0` and `-O2`. Constructing that
stress test surfaced an unrelated, branch-only segfault on a discarded comma
operand naming the postcondition result, since fixed, whose reproducer now
lives in
`gcc/testsuite/g++.dg/contracts/cpp26/postcondition-discarded-operand-dependent.C`.
