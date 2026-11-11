# GCC-17: `this` accepted in the trailing return type of an explicit-object member function's own declaration

**Status:** Open
**Component:** c++ / parser
**Keywords (ours -- upstream sets its own):** `accepts-invalid`
**Upstream Link:** [PR127290](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127290)
-- **FILED 2026-09-09**, UNCONFIRMED. (Searched 2026-09-05 before filing,
including resolved bugs: nothing matched.)
**Affects:** measured 2026-09-09 -- accepted on 14.4.0, 15.3.0, 16.1.0,
16.2.0 and both trunk builds. 13.4.0 does not support explicit object
parameters at all. Not contracts-related.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `this is accepted in the trailing return type of an explicit object member function` |

Attachments:

| File | Description |
|---|---|
| [`this-in-xobj-trailing-return.cpp`](this-in-xobj-trailing-return.cpp) | The accepted explicit-object case, with the rejected static control and an implicit-object case that must compile |

````
[expr.prim.this]/3, as amended by P0847R7 (deducing this):

  "It shall not appear within the declaration of either a static member
   function or an explicit object member function of the current class
   (although its type and value category are defined within such member
   functions as they are within an implicit object member function)."

A trailing return type is within the declaration, so `this` is ill-formed
in it for both static member functions and explicit object member
functions.  GCC rejects the static half and accepts the explicit-object
half.

```
struct Bug {
    int x;
    // Accepted, and should not be.
    auto f(this Bug& self) -> decltype(this->x);
};

struct Control {
    int x;
    // The same shape on a static member function, governed by the same
    // sentence.  Rejected -- so the rule is implemented, and applied to
    // only half of what it says.
    static auto g() -> decltype(this->x);
};

struct MustCompile {
    int x;
    // An implicit object member function may of course name `this' here.
    auto h() -> decltype(this->x);
};
```

```
$ ./gcc-16.2.0/bin/g++ -std=c++23 -fsyntax-only \
    this-in-xobj-trailing-return.cpp
this-in-xobj-trailing-return.cpp:33:33: error: invalid use of 'this' at top level
   33 |     static auto g() -> decltype(this->x);
      |                                 ^~~~
this-in-xobj-trailing-return.cpp:33:33: error: invalid use of 'this' at top level
```

The only diagnostic is for Control::g.  Bug::f -- the same shape, governed
by the same sentence -- is accepted.

DISCOVERY

Found while implementing C++26 contract predicates on explicit object member
functions, where the analogous question -- may a predicate name `this`? --
had to be settled.  Answering it meant reading [expr.prim.this]/3 closely
enough to notice that its static and explicit-object halves are enforced
differently outside contracts too.  Nothing here involves contracts.

VERSIONS -- all on x86_64-linux-gnu

  source              version                       accepts Bug::f
  compiler-explorer   14.4.0                        yes
  compiler-explorer   15.3.0                        yes
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

[`this-in-xobj-trailing-return.cpp`](this-in-xobj-trailing-return.cpp)
is the attachment: the one failing row plus its two controls, in one
compile.

[`gcc-17-this-in-xobj-declaration.cpp`](gcc-17-this-in-xobj-declaration.cpp)
is retained for `verify.sh`, which needs one measurement per context and so
selects them with `-DCASE=1..11`; a whole-file measurement would average the
enforced and unenforced halves into one word.

