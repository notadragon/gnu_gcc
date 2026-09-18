# GCC-37: A `std::source_location` that is not a class ICEs the contract machinery

**Status:** Fixed here (not by a fix -- see below)
**Resolved by:** `2600-source-location-machinery` (which deletes the
`lookup_std_type` path the ICE lives in) on top of `2200-libcontracts`
(which supplies the POD ABI field that replaces it)
**Component:** c++ / contracts
**Upstream Link:** [PR127255](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127255)
-- filed 2026-09-08 by a third party, UNCONFIRMED, keywords `c++-contracts`
`ice-on-invalid-code`. Not ours; found while reading Bugzilla.

**Affects:** stock trunk (17.0.0 20260905). Reproduced on
`compilers/gcc-trunk`.

## Bug Report

When a contract is checked, stock builds a `contract_violation` and needs the
layout of `std::source_location`. `get_contracts_source_location_impl_type`
(`gcc/cp/contracts.cc:2684`) does:

```c
  tree contracts_source_location_type
    = lookup_std_type (get_identifier ("source_location"));

  if (contracts_source_location_type
      && contracts_source_location_type != error_mark_node
      && TYPE_FIELDS (contracts_source_location_type))
```

`TYPE_FIELDS` requires a `RECORD_TYPE`, `UNION_TYPE` or `QUAL_UNION_TYPE`.
Nothing establishes that the name found is any of those. If the program
declares `std::source_location` as something else, the tree check fires:

```
internal compiler error: tree check: expected record_type or union_type or
qual_union_type, have integer_type in get_contracts_source_location_impl_type,
at cp/contracts.cc:2695
```

Two shapes, both in the PR:

* `namespace std { using source_location = int; }` -- an alias to a non-class
  (`gcc-37a-source-location-alias-ice.cpp`).
* `namespace std { union source_location {}; }` -- a **different ICE site**
  (`gcc-37b-source-location-union-ice.cpp`).  A union satisfies the tree check
  above, so this one gets as far as diagnosing the missing member and then
  crashes on the error node it produced:

  ```
  error: '__impl' is not a member of 'std::source_location'
  internal compiler error: tree check: expected record_type or union_type or
  qual_union_type, have error_mark in build_source_location_impl,
  at cp/cp-gimplify.cc:4388
  ```

  So the PR is really two defects sharing a cause: `contracts.cc:2695` does not
  check that the lookup found a class, and `cp-gimplify.cc:4388` does not check
  that it found anything valid at all.  A guard at only the first site would
  leave the union shape crashing.

Both programs are ill-formed by [namespace.std] -- adding declarations to
`namespace std` is undefined -- so this is ice-on-invalid rather than
ice-on-valid. That lowers its priority but not its wrongness: an ICE is never
the right diagnosis, and the reporter arrived here from a real codebase that
aliases the name.

## Why this branch does not reproduce it

**Not because we fixed it. Because we deleted the code it lives in.**

Stock's front end reaches into `std::source_location`'s layout to build the
violation object. This branch does not: a contract's source location is
emitted as a plain POD field of the ABI data block (`bits/contracts_abi.h`,
`CXA_FIELD_SOURCE_LOCATION`), built from `location_t`, and the *library*
converts it at the point of use --
`contract_violation::location()` calls
`std::source_location::__create_from_pointer`. The front end never calls
`lookup_std_type (get_identifier ("source_location"))` at all;
`grep lookup_std_type gcc/cp/contracts.cc` is empty here and has two hits on
stock.

So the branch is immune to whatever the user declares under that name, and the
immunity is structural rather than defensive.

## Measurement

| | stock trunk | this branch |
|---|---|---|
| alias-to-`int` shape | **ICE** | accepted |
| `union` shape | **ICE** | accepted |

## What upstream should do

Two guards, not one, because there are two crash sites:

* `contracts.cc:2695` -- test `CLASS_TYPE_P` before `TYPE_FIELDS`, falling back
  to the synthesized layout stock already builds when the lookup finds nothing
  usable; that path exists immediately below the failing check.
* `cp-gimplify.cc:4388` (`build_source_location_impl`) -- bail out on
  `error_mark_node` rather than tree-checking it, which is what the union shape
  reaches after its own diagnostic.

Guarding only the first leaves the union shape crashing, which is worth saying
explicitly on the PR: the two shapes in it are not the same bug twice.

The broader observation is the one this branch acts on: making contract
violation construction depend on the *layout* of a library type the user can
redeclare is fragile, and routing it through a POD ABI field removes the
dependency entirely.
