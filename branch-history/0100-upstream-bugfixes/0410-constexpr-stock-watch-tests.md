---
id: 0410-constexpr-stock-watch-tests
subject: 'c++: record constant-evaluation bugs found by a per-UB review'
depends: []
regenerates: []
fixes: []
---

## Rationale

Not fixed here; this commit only records the bugs.  A review of every
category of core-language undefined behavior found constructs that
constant evaluation accepts although their behavior is undefined, so the
enclosing expression is not a core constant expression: forming a pointer
beyond one past the end of an array, subtracting pointers into different
members, out-of-range enumeration casts, pointer-to-member casts and uses
involving a class that lacks the member, volatile access through a
non-volatile glvalue, creating an object in a const object, uses of
objects outside their lifetime (double destruction, member calls,
`typeid`, `dynamic_cast`, conversions to a virtual base), unsequenced
modification, null and past-the-end indirection whose result is not read,
`delete[]` through a base pointer, modifying a `new const` object, members
named in a constructor's function-try-block handler, uses of the object
under construction before its bases are (a member call, `dynamic_cast` or
virtual-base conversion in a base mem-initializer), a call to a pure
virtual function from its constructor, references to deallocated or
dangling storage, overlapping union assignment, `delete` of allocator
storage, and a placement new of an empty class at a null pointer.  It also found two ICEs reached while diagnosing (a
`static_cast` from `void *` of `a + N`; explaining why a destructor with a
virtual base is not constexpr), a rejects-valid pointer-to-member use, an
unhelpful diagnostic for an out-of-range floating-point enumeration cast,
and a false `-fsanitize=vptr` report for a downcast made during
construction.

Each has a watch test: an xfailed `dg-error` (or `dg-bogus`, `dg-message`,
`dg-ice`, `dg-output`) where the right behavior differs from today's, so a
fix shows as an XPASS.  The ICE tests reach the ICE without an earlier
error, so a release compiler reports it too rather than "confused by
earlier errors".  Three tests pin behaviour GCC already gets right on
programs where Clang does not: a past-the-end pointer converted to a
non-virtual base, a `[[noreturn]]` function that returns, and a
`dynamic_cast` through a virtual base during construction.  One pins a
correct rejection that looks like a bug: copying a padding byte from a
`std::bit_cast` result into an `unsigned char` is well-defined at run
time, but [expr.const] excludes an lvalue-to-rvalue conversion of an
indeterminate value from constant evaluation.  Tracked as
GCC-711..GCC-717, GCC-739..GCC-764, GCC-779 and GCC-784..GCC-786 in
`bug-reports/`
and `open-issues/README.md`.  The `fixes:` list is empty on purpose.

## Compile gap

None.  Pure test additions against upstream behaviour; they use no
contracts and nothing this branch adds.  They sit under
`g++.dg/contracts/cpp26/` with the other watch tests, and the sanitizer
one under `g++.dg/ubsan/`.

## Contents

- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-dynamic-cast-vbase-construction.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-indeterminate-uchar-copy.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-noreturn-returns.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-past-end-nonvirtual-base.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-address-past-end-indirection.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-array-delete-via-base.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-call-in-base-mem-init.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-cdtor-convert-typeid-virtual.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-cdtor-dynamic-cast.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-create-in-const-object.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-delete-allocator-storage.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-double-destruction-automatic.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-dynamic-cast-past-end-downcast.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-dynamic-cast-typeid-outside-lifetime.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-empty-store-past-end.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-enum-floating-cast-diagnostic.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-enum-out-of-range.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-function-try-block-handler-member.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-member-call-outside-lifetime.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-modify-new-const.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-null-data-member-pointer-address.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-null-indirection-no-read.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-oob-pointer-formation.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-placement-new-null-empty.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-ptrmem-cast-to-base-rejects-valid.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-ptrmem-cast-unrelated-base.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-ptrmem-object-lacks-member.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-pure-virtual-direct-call.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-reference-reinterpret-cast.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-reference-self-init.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-reference-through-dangling-pointer.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-reference-through-invalid-pointer.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-reference-to-freed-storage.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-sub-same-object.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-union-overlap-assignment.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-unsequenced-modification.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-vbase-conversion-destroyed-named.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-vbase-dtor-explain-ice.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-virtual-call-base-mem-init.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-void-cast-pointer-plus-ice.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-volatile-through-nonvolatile.C : *
- gcc/testsuite/g++.dg/ubsan/open-bug-vptr-downcast-during-construction.C : *
