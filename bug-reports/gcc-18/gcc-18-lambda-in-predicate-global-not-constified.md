# GCC-18: Global named inside a lambda within a predicate is not constified

**Status:** Fixed here (commit [83c061ffdbd0](https://github.com/notadragon/gnu_gcc/commit/83c061ffdbd036002e0314b070b1006606d8f2c1))
**Resolved by:** `1050-predicate-lambda-constify`
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `accepts-invalid`
**Upstream Link:** [PR127293](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127293)
-- **FILED 2026-09-09**, UNCONFIRMED. (Searched 2026-09-05 before filing,
including resolved bugs: nothing matched.)
**Affects:** measured 2026-09-09 -- reproduces on 16.1.0, 16.2.0 and both
trunk builds.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] a namespace-scope variable named inside a lambda within a contract predicate is not made const` |

Attachments:

| File | Description |
|---|---|
| [`lambda-in-predicate-global-not-constified.cpp`](lambda-in-predicate-global-not-constified.cpp) | [expr.prim.id.unqual]/3's own example: only ++i is diagnosed, ++n is not |

````
[expr.prim.id.unqual]/3 makes an id-expression naming a variable const when
it appears in a contract predicate, and its own example covers the case
where the naming happens inside a lambda written within that predicate.  GCC
applies this to the lambda's captures but not to a variable of namespace
scope.

This is the example from that paragraph, with the outcome it requires on
each line:

```
int n = 0;

struct X { bool m(); };

struct Y {
    int z = 0;

    void f(int i, int* p, int& r, X x, X* px)
        pre([=, &i, *this] mutable {
            ++n;         // error expected: attempting to modify const lvalue
            ++i;         // error expected: attempting to modify const lvalue
            ++p;         // OK, refers to member of closure type
            ++r;         // OK, refers to non-reference member of closure type
            ++this->z;   // OK, captured *this
            ++z;         // OK, captured *this
            return true;
        }())
    {}
};
```

```
$ ./gcc-16.2.0/bin/g++ -std=c++26 -fsyntax-only \
    lambda-in-predicate-global-not-constified.cpp
lambda-in-predicate-global-not-constified.cpp: In lambda function:
lambda-in-predicate-global-not-constified.cpp:22:15: error: increment of read-only reference 'i'
   22 |             ++i;         // error expected: attempting to modify const lvalue
      |               ^
```

++i is diagnosed and ++n is not.  The four lines that must keep compiling do.

When named directly in the predicate rather than through a lambda, the very
same variable is constified correctly.

```
$ cat direct.cpp
int n = 0;
void f() pre((++n, true)) { }

$ ./gcc-16.2.0/bin/g++ -std=c++26 -fsyntax-only direct.cpp
direct.cpp:2:17: error: increment of read-only location '(const int)n'
    2 | void f() pre((++n, true)) { }
      |                 ^
```

The rule needs to be properly applied within the lambda body in the same way it
is applied in the rest of the predicate.

DISCOVERY

Found while validating the examples from the standard in
[expr.prim.id.unqual]/3.

ANALYSIS

The constification walks the predicate and rewrites id-expressions naming
variables.  Inside a lambda it consults the closure: a capture is rewritten
through the corresponding member, which is why ++i and the four OK rows all
behave.  A namespace-scope variable has no capture and no closure member, so
the walk finds nothing to rewrite and leaves the reference alone.

VERSIONS -- all on x86_64-linux-gnu

  source              version                       accepts ++n
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

See [`lambda-in-predicate-global-not-constified.cpp`](lambda-in-predicate-global-not-constified.cpp)
in this directory.

## Our Fix

`gcc/cp/*`: the constification check now looks for an *enclosing* contract
scope rather than only the innermost binding level, while still exempting
lambda-capture proxies and predicate-local declarations.

## Notes

Mirror image of an already-fixed Clang bug in the companion Clang fork,
where Clang under-constified only automatic-storage variables in the same
way; worth cross-referencing when filed.
