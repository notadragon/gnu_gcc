# GCC-21: `-std=c++26` alone does not link the violation handler

**Status:** Fixed here -- but not via an isolated commit; see Notes.
**Resolved by:** no single commit.  The `OPT_std_c__26` / `OPT_std_gnu__26`
cases of `g++spec.cc`'s `need_experimental` switch land with
`2200-libcontracts`, and the per-paper options only imply `-fcontracts` once
`6900-cross-paper-options` lands -- but both of those commits exist for their
own reasons and neither is a fix for this defect, so the table links neither.
There is no discrete fix commit in the fork's own history either; see Notes.
**Component:** driver / `g++spec.cc`
**Upstream Link:** [PR126158](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126158)
**Affects:** stock g++ 16.2.0 and g++ trunk (17.0.0 20260901); both fail to
link a plain `-std=c++26` contracts program

## Bug Report

Compiling and linking a C++26 program that uses contracts with just
`-std=c++26` (no `-fcontracts`, no explicit runtime library) fails at link
time:

```
undefined reference to `handle_contract_violation
  (std::contracts::contract_violation const&)'
```

`-std=c++26` implicitly enables `-fcontracts` in the front end, so the
program compiles, but the driver does not know that C++26 mode alone
implies a need for the experimental runtime library (`libstdc++exp`, which
hosts the contracts C++ runtime and the default violation handler) or the
contracts ABI core (`libcontracts`). The user must pass `-lstdc++exp`
explicitly to link, which is a surprising and undocumented requirement:
nothing else about enabling C++26 mode requires manually naming a runtime
library.

## Reproducer

See [`gcc-21-cpp26-missing-experimental-link.cpp`](gcc-21-cpp26-missing-experimental-link.cpp)
in this directory. On stock g++ 16.2.0:

```
$ g++ -std=c++26 gcc-21-cpp26-missing-experimental-link.cpp -o out
undefined reference to `handle_contract_violation(std::contracts::contract_violation const&)'
undefined reference to `handle_contract_violation(std::contracts::contract_violation const&)'
collect2: error: ld returned 1 exit status
$ g++ -std=c++26 gcc-21-cpp26-missing-experimental-link.cpp -o out -lstdc++exp
$ ./out; echo $?
0
```

The same happens verbatim on g++ trunk (17.0.0 20260901).

## Our Fix

Built into this fork's `gcc/cp/g++spec.cc` since its original base
implementation commit, not a discrete fix commit -- see Notes for how this
was confirmed. `lang_specific_driver`'s `need_experimental` switch
includes `OPT_std_c__26` and `OPT_std_gnu__26` (alongside `OPT_std_c__29`
and `OPT_std_gnu__29`) among the cases that auto-link `-lstdc++exp` and the
contracts ABI core, on the reasoning that any use of C++ contracts needs
the experimental runtime, and C++26 mode alone is one of the ways
`-fcontracts` gets enabled.

## Notes

There is no isolable "fix commit" for this bug: it has been present in
this branch's `g++spec.cc` since the branch's very first contracts-related
commit. Confirmed via:

```
$ git log --oneline -S "OPT_std_c__26" -- gcc/cp/g++spec.cc
97fd9dcfb24 c++, contracts: P3850 contracts extensions
$ git show 97fd9dcfb24:gcc/cp/g++spec.cc | grep -A3 "OPT_std_c__26"
	case OPT_std_c__26:
	case OPT_std_gnu__26:
	  /* Any use of C++ contracts needs the experimental runtime ... */
```

This branch's original base implementation commit ("P3850 contracts
extensions") already carries `OPT_std_c__26`/
`OPT_std_gnu__26` in the `need_experimental` switch, alongside the P3850
paper flags. There is no earlier or later commit that specifically
introduces this handling for the C++26 spellings -- it was present from
day one of this fork's own history. (The C++29 spellings are a separate,
later addition; see below.)

This repository does not have a plain, unmodified upstream GCC checkout to
diff against directly; the confirmation that stock GCC lacks this handling
is the empirical link failure reproduced above against the standalone
release binaries `gcc-16.2.0/bin/g++` and `gcc-trunk/bin/g++`, both of
which are unmodified stock builds.

There is a related defect one standard version along: the C++29 half of
what PR126158 is the C++26 half of. Unlike PR126158/GCC-21, that one does
have a discrete fix commit ("driver: link the contracts runtime for
-std=c++29 too"), which adds `OPT_std_c__29`/`OPT_std_gnu__29` to
`g++spec.cc`'s `need_experimental` switch. The C++29 spellings are
genuinely absent from the branch's original base implementation commit,
contrary to what an earlier note here implied, and only that later commit
introduces them. So PR126158/GCC-21 (no discrete commit; C++26 support was
complete from the base implementation onward) and the C++29 gap (a real,
separate one closed later) are two genuinely separate defects, not two
halves of the same fix.
