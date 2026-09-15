# GCC-29: The `__contract_assert` extension spelling ICEs in grok_contract

**Status:** Fixed here (commit `f64b7bbafac`)
**Resolved by:** `4620-contract-extension-fields` -- that commit owns
`grok_contract` whole, so the spelling check travels with it
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `ice-on-valid-code`
**Upstream Link:** [PR127295](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127295)
-- **FILED 2026-09-09**, UNCONFIRMED. (Searched 2026-09-05 before filing,
including resolved bugs: comment text `__contract_assert` returns
**nothing at all** -- the extension spelling is not mentioned in any GCC
bug -- and `grok_contract` returns only PR108542 and PR113968, both
long-fixed ICEs elsewhere in that function.)
**Affects:** measured 2026-09-09 -- ICEs on 16.1.0, 16.2.0 and both trunk
builds. The assertion is at `cp/contracts.cc:2068` on 16.x and `:2102` on
trunk.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] ICE in grok_contract on the __contract_assert extension spelling` |

Attachments:

| File | Description |
|---|---|
| [`contract-assert-alt-spelling-ice.cpp`](contract-assert-alt-spelling-ice.cpp) | ICE on __contract_assert, with the standard spelling as a control |

````
GCC accepts __contract_assert as its own extension spelling of
contract_assert, and then ICEs on it.

```
void f(int x)
{
    __contract_assert(x > 0);   // ICE
}

// CONTROL: the standard spelling is fine, which is what places the defect in
// recognising the token rather than in assertion-statements generally.
void g(int x)
{
    contract_assert(x > 0);
}
```

```
$ ./gcc-16.2.0/bin/g++ -std=c++26 -fsyntax-only \
    contract-assert-alt-spelling-ice.cpp
contract-assert-alt-spelling-ice.cpp: In function 'void f(int)':
contract-assert-alt-spelling-ice.cpp:12:27: internal compiler error: in grok_contract, at cp/contracts.cc:2068
   12 |     __contract_assert(x > 0);   // ICE
      |                           ^
0x78ce6002a1c9 __libc_start_call_main
	../sysdeps/nptl/libc_start_call_main.h:58
0x78ce6002a28a __libc_start_main_impl
	../csu/libc-start.c:360
```

The standard spelling in g() is accepted, which is what places the defect in
recognising the token rather than in assertion-statements generally.

From a trunk build (7dab38c9d71) configured the same way, but with -g, we
get a more complete stack trace:

```
0x24e67c5 internal_error(char const*, ...)
	gcc/diagnostic-global-context.cc:787
0x824eb1 fancy_abort(char const*, int, char const*)
	gcc/diagnostics/context.cc:1813
0x7bec9f grok_contract(tree_node*, tree_node*, tree_node*, cp_expr, unsigned long)
	gcc/cp/contracts.cc:2102
0x9a67b4 cp_parser_contract_assert
	gcc/cp/parser.cc:33941
0x9a67b4 cp_parser_statement
	gcc/cp/parser.cc:14457
0x9a7e9e cp_parser_statement_seq_opt
	gcc/cp/parser.cc:15140
0x9a8097 cp_parser_compound_statement
	gcc/cp/parser.cc:14987
0x9da51e cp_parser_function_body
	gcc/cp/parser.cc:29010
0x9da51e cp_parser_ctor_initializer_opt_and_function_body
	gcc/cp/parser.cc:29061
0x9a82ab cp_parser_function_definition_after_declarator
	gcc/cp/parser.cc:36364
```

DISCOVERY

Found by an audit comparing every test in a C++26 contracts implementation's
own suite against stock trunk, looking for cases that pass locally because
they were fixed locally.  This one had been fixed without ever being
classified as upstream's.

ANALYSIS

c-common.cc maps both __contract_assert and the standard contract_assert to
RID_CONTASSERT, so the two arrive at grok_contract indistinguishably as far
as the parser is concerned.  grok_contract then tested only for the standard
spelling and fell off the end of its if/else chain into the trailing
assertion.

So this is an ICE on a spelling the compiler itself defines, reachable with
nothing but a C++26 mode.

VERSIONS -- all on x86_64-linux-gnu

  source              version                       ICE
  compiler-explorer   16.1.0                        yes (contracts.cc:2068)
  compiler-explorer   16.2.0                        yes (contracts.cc:2068)
  compiler-explorer   17.0.0 20260909, 919c0d16c91  yes (contracts.cc:2102)
  local build -g      17.0.0 20260909, 7dab38c9d71  yes (contracts.cc:2102)

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

See [`contract-assert-alt-spelling-ice.cpp`](contract-assert-alt-spelling-ice.cpp)
in this directory, which pairs the ICE with the standard spelling as a
control -- that is what places the defect in token recognition rather than in
assertion-statements generally.

## Our Fix

Recognise both spellings in `grok_contract`.

Test: `gcc/testsuite/g++.dg/contracts/cpp26/contract-assert-alt-spelling.C`,
which also covers the spelling inside a template and inside a lambda's own
contract, since those reach `grok_contract` by different paths.

## Notes

Found by the 2026-09-05 audit comparing every plain-`-fcontracts` test in our
suite against stock trunk. It had been fixed on the branch without ever being
classified as upstream's, which is the same gap that hid
[GCC-28](../gcc-28/gcc-28-xobj-member-in-predicate-ctor-message.md).
