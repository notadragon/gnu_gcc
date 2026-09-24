# GCC-10: Contract predicate on a `this`-capturing lambda reads the raw closure object

**Status:** Fixed here (commit [984f2e891022](https://github.com/notadragon/gnu_gcc/commit/984f2e8910220b14872e6c27cfae9b32d04c7b46))
**Resolved by:** `1180-lambda-this-capture-remap`
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `wrong-code`
**Upstream Link:** [PR127283](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127283) -- **FILED 2026-09-09**, UNCONFIRMED. (Searched
2026-09-05 before filing, including resolved bugs, on comment text
`contract lambda capture this closure`: the one hit, PR124958, is RESOLVED
INVALID and about IPA-SRA with `do_not_optimize`, not this.)
**Affects:** measured 2026-09-09 -- reproduces on 16.1.0, 16.2.0 and both
trunk builds; fixed on this branch.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] a contract predicate on a this-capturing lambda reads the closure object as if it were the enclosing class` |

Attachments:

| File | Description |
|---|---|
| [`lambda-this-capture-in-contract.cpp`](lambda-this-capture-in-contract.cpp) | Predicate reads the closure instead of the captured object: exits 1 |

````
A contract predicate on a lambda that captured `this` reads the closure
object as if it were the enclosing class.  It compiles without a diagnostic
and produces wrong code.

```
static int seen = -1;

static bool probe(int observed) { seen = observed; return true; }

struct S {
    int m;

    int through_this()
    {
        // The predicate reads m through the captured `this'.
        auto l = [this]() pre(probe(m)) { return m; };
        return l();
    }
};

int main()
{
    S s{2};
    int body = s.through_this();
    // The body reads m correctly; the predicate should see the same value.
    return (body == 2 && seen == 2) ? 0 : 1;
}
```

```
$ ./gcc-16.2.0/bin/g++ -std=c++26 lambda-this-capture-in-contract.cpp \
    -lstdc++exp -o t
$ ./t; echo $?
1
```

The body reads m correctly and sees 2; the predicate, evaluated on entry to
the same call, does not.

The symptom is nondeterministic if you only watch whether the check fires --
the predicate ends up comparing against a stack address, so whether any
given comparison holds varies between builds.  That is why the reproducer
records the value the predicate saw rather than whether it fired.

The generated operator() shows it directly:

```
_1 = MEM[(struct S *)__closure].m;      // the predicate -- WRONG
_2 = __closure->__this; _3 = _2->m;     // the body      -- right
```

DISCOVERY

Found while implementing capture of enclosing entities by a lambda written
inside a contract predicate ([expr.prim.lambda.capture]/3.3, PR117435), in
the regression test for that work.  It is an older and separate defect that
the new test happened to reach.  An earlier compile-only probe of this shape
had reported "ok", which is exactly the trap described above: nothing fires
reliably, so nothing looks wrong.


ANALYSIS

remap_dummy_this_1 (gcc/cp/contracts.cc) rewrites every tree for which
is_this_parameter is true to DECL_ARGUMENTS of the function being emitted
into.  In a lambda's operator() that first argument is __closure, not an S*,
so the member access reinterprets the closure object.

is_this_parameter (gcc/cp/semantics.cc) is deliberately true for BOTH the
real `this` PARM_DECL and a lambda's captured-`this` proxy -- a VAR_DECL
named `this` whose DECL_VALUE_EXPR is already `__closure->__this`.  The proxy
needs no remapping at all: the remap exists for the DUMMY `this` of a
contract parsed on a declaration, and for re-pointing a real `this` at the
first parameter of an outlined checking function.  Both of those are
PARM_DECLs.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       wrong code
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

See [`lambda-this-capture-in-contract.cpp`](lambda-this-capture-in-contract.cpp)
in this directory.

## Our Fix

`gcc/cp/contracts.cc`: restrict the `remap_dummy_this_1` rewrite to
`PARM_DECL`s only, leaving the proxy `VAR_DECL`'s existing
`DECL_VALUE_EXPR` to resolve correctly.

## Notes

Filed 2026-09-09 as [PR127283](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127283), UNCONFIRMED.

A silent-miscompile bug: it produces no crash and no diagnostic, which the
report says up front for that reason.
