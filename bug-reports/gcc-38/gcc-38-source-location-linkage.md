# GCC-38: An unrelated precondition gives `std::source_location` internal linkage

**Status:** Fixed here (not by a fix -- see below)
**Resolved by:** `2600-source-location-machinery` (which deletes the
`lookup_std_type` path) on top of `2200-libcontracts` (which supplies
the POD ABI field that replaces it)
**Component:** c++ / contracts
**Upstream Link:** [PR127251](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127251)
-- filed 2026-09-08 by a third party, UNCONFIRMED, keywords `c++-contracts`
`c++26` `diagnostic`. Not ours; found while reading Bugzilla.

**Affects:** stock trunk (17.0.0 20260905). Reproduced on
`compilers/gcc-trunk`.

## Bug Report

A precondition anywhere in the translation unit changes the linkage of
`std::source_location` for the rest of it -- if the contract is seen before
`<source_location>` is included.

```cpp
// foo.hpp
void fn() pre(true) {}

// bar.hpp
#include <source_location>
struct bar { std::source_location loc; };

// main.cpp
#include "foo.hpp"
#include "bar.hpp"
int main() {}
```

```
bar.hpp:2:8: warning: 'bar' has a field 'std::source_location bar::loc' whose
type has internal linkage [-Wsubobject-linkage]
```

The reporter notes three transformations that each make it go away: deleting
`fn`'s definition, deleting the precondition, or swapping the include order.
The last is the tell -- the contract is what pulls the type into existence
early.

Cause: building the violation object makes stock resolve
`std::source_location` through
`get_contracts_source_location_impl_type` -> `get_source_location_impl_type`
(`gcc/cp/contracts.cc:2684`, `:2697`). Reached before the real header is seen,
that synthesizes the type rather than finding it, and the synthesized one has
internal linkage. The subsequent `#include <source_location>` does not repair
the linkage of what is already there.

`-Wsubobject-linkage` is the visible symptom, and it is a warning, so it is
easy to read this as cosmetic. It is not: the linkage of a standard library
type has silently changed because of an unrelated declaration elsewhere in the
TU, which is an ODR hazard across translation units that include the two
headers in different orders.

## Why this branch does not reproduce it

Same structural reason as [GCC-37](../gcc-37/gcc-37-source-location-alias-ice.md), and
the two are worth reading together -- one report is an ICE and the other a
linkage change, but both are the front end reaching for
`std::source_location` while building a violation.

This branch never resolves that type. The contract's source location is a POD
field of the ABI data block (`CXA_FIELD_SOURCE_LOCATION`), built from
`location_t`; `contract_violation::location()` converts it in the library via
`std::source_location::__create_from_pointer`. Nothing in the front end
mentions the name, so nothing brings the type into existence early and its
linkage is whatever the real header says.

## Measurement

Three-file reproducer under `gcc-38-files/`, measured with
`-std=c++26 -fcontracts`:

| | stock trunk | this branch |
|---|---|---|
| `-Wsubobject-linkage` on `bar` | **warns** | silent |

The single-file collapse of this reproducer does **not** show the bug --
`-Wsubobject-linkage` concerns a class defined in a header, so the three files
are load-bearing. A first attempt at a one-file version measured clean on
stock and would have been recorded as "does not reproduce" had it been
trusted.

## No testsuite pin

GCC-37 has two regression pins in the suite
(`g++.dg/contracts/cpp26/contract-source-location-{alias,union}.C`), which
catch the front end reacquiring a `std::source_location` lookup on the contract
path.  This entry has none.

`-Wsubobject-linkage` only fires for a class defined in a *header*, so pinning
it needs a multi-file test with a real include, and the single-file collapse
measures clean even on stock -- it would be a test that passes for the wrong
reason.  The reproducer under `gcc-38-files/` is measured by `verify.sh`
instead, which is where the stock-vs-branch comparison belongs anyway.  If the
GCC-37 pins ever start failing, this one is failing too, since both have the
same cause.

## What upstream should do

Deferring the resolution until the type is actually needed would narrow the
window but not close it -- a TU that never includes `<source_location>` still
has to get something. The real answer is not to synthesize a library type from
the front end at all, which is what this branch does by carrying the location
as a POD across the ABI.
