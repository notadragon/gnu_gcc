# GCC-15: Outlined contract checks lose by-value mutations under `-fcontract-checks-outlined`

**Status:** Fixed here (commit [375ce01479ce](https://github.com/notadragon/gnu_gcc/commit/375ce01479ce2cecf139b04de05e203f5aa36e40))
**Resolved by:** `1120-outlined-checks-by-reference` (passing by-value parameters by
reference, in `build_contract_condition_function`) together with
`4000-p3098` (the matching `remap_as_value` read in `remap_contract`
and `setup_param_remap`); neither half works alone
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `wrong-code`
**Upstream Link:** [PR127289](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127289)
-- **FILED 2026-09-09**, UNCONFIRMED. (Searched 2026-09-05 before filing,
including resolved bugs: nothing matched. Upstream demonstrably knows: the
upstream testsuite carries the failing case marked
`{ dg-xfail-run-if "PRXXXXXX" { *-*-* } }` -- a placeholder where a bug
number belongs -- so this is a clean filing rather than a duplicate.)
**Affects:** measured 2026-09-09 -- 16.1.0, 16.2.0 and both trunk builds all
give 0 inlined and 19 outlined.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] -fcontract-checks-outlined loses a predicate's mutations of by-value parameters and of the postcondition result` |

Attachments:

| File | Description |
|---|---|
| [`outlined-checks-lose-by-value-mutations.cpp`](outlined-checks-lose-by-value-mutations.cpp) | Exits 0 as compiled, 19 under -fcontract-checks-outlined; the status is a per-entity bitmask |

````
Under -fcontract-checks-outlined a contract predicate that mutates a
by-value parameter writes to a copy, so neither the function body nor the
caller sees the mutation.  The same happens to a postcondition's result
binding.  Without the flag the mutation is observed.  So the observable
behaviour of a conforming program depends on a codegen flag.

Exactly the entities passed by value to the outlined check are lost:

  entity mutated in the predicate          inlined    outlined
  a global                                 observed   observed
  a by-value parameter                     observed   lost
  a reference parameter                    observed   observed
  the result binding in a post             observed   lost

```
int g = 0;

int by_value_parm(int n) pre(const_cast<int&>(n)++)
{
    return n;                  // required: 3.  Outlined gives 2.
}

int result_binding() post(r : const_cast<int&>(r)++)
{
    return 1;                  // caller sees 2 inlined, 1 outlined.
}

int ref_parm(int& n) pre(const_cast<int&>(n)++)
{
    return n;
}

void global_mut() pre(const_cast<int&>(g)++)
{
}

// The crux: one predicate that mutates both shared state and the by-value
// parameter.  Elision would drop both; this drops only one.
int g_log = 0;

bool bump(int& n)
{
    ++n;
    g_log = n;                 // shared state: propagates even when outlined
    return true;
}

int partial_application(int n) pre(bump(const_cast<int&>(n)))
{
    return n * 100 + g_log;    // faithful 303, fully elided 200, outlined 203
}

int main()
{
    int bad = 0;

    int r = partial_application(2);
    if (r != 303 && r != 200)
        bad |= 16;             // 203: neither faithful nor elided

    if (by_value_parm(2) != 3)
        bad |= 1;              // fires only when outlined

    if (result_binding() != 2)
        bad |= 2;              // fires only when outlined

    int v = 2;
    ref_parm(v);
    if (v != 3)
        bad |= 4;              // never fires

    g = 3;
    global_mut();
    if (g != 4)
        bad |= 8;              // never fires

    return bad;
}
```

```
$ ./gcc-16.2.0/bin/g++ -std=c++26 \
    outlined-checks-lose-by-value-mutations.cpp -lstdc++exp -o t
$ ./t; echo $?
0

$ ./gcc-16.2.0/bin/g++ -std=c++26 -fcontract-checks-outlined \
    outlined-checks-lose-by-value-mutations.cpp -lstdc++exp -o t
$ ./t; echo $?
19
```

This is not a behavior allowed by predicate elision, as not all
side effects of the predicate are being elided.  In the above code,
a result of 200 (no evaluations) or 303 (both side effects) would be
valid, but with -fcontract-checks-outlined the result is 203.

DISCOVERY

Found by working through the interaction of the evaluation-semantic rules
with GCC's own -fcontract-checks-outlined codegen mode, asking for each kind
of entity a predicate can mutate whether the two lowerings agree.  Four of
the five agree; the two passed by value do not.

ANALYSIS

build_contract_condition_function (gcc/cp/contracts.cc) builds the outlined
checking function by copy_decl-ing each of the original's PARM_DECLs with
their types unchanged, so a by-value int n is still by-value in the checking
function.  For postconditions the result is appended as one more by-value
parameter of the original return type.

VERSIONS -- all on x86_64-linux-gnu

  source              version                       inlined  outlined
  compiler-explorer   16.1.0                        0        19
  compiler-explorer   16.2.0                        0        19
  compiler-explorer   17.0.0 20260909, 919c0d16c91  0        19
  local build -g      17.0.0 20260909, 7dab38c9d71  0        19

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

See [`outlined-checks-lose-by-value-mutations.cpp`](outlined-checks-lose-by-value-mutations.cpp)
in this directory.

## Our Fix

`gcc/cp/contracts.cc` (`build_contract_condition_function`): pass by-value
parameters by reference into the outlined check functions.

## Notes

This fix retires the upstream `expr.prim.id.unqual.p7-3.C` xfail, which is
worth noting explicitly when this is filed or referenced upstream.
