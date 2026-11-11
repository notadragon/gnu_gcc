# GCC-502: A constructor-precondition / destructor-postcondition call that selects a static overload is rejected

**Status:** Open (upstream)
**Resolved by:** `1200-xobj-member-in-predicate`
**Component:** c++
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST, summary
and comment text, on `contract static member overload`, `constructor
precondition static` and the diagnostic text; the only hit, PR127294, is the
explicit-object member function case of the same diagnostic, not this.
Earlier search 2026-09-25, same result)
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0, stock trunk
`17.0.0 20260922` (`f008f03eff8`) and this branch all reject both cases
(review round #12 fan-out, c-b-3)
**Clang:** has the same defect (CLANG-502 in that fork)

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] a call in a constructor precondition that resolves to a static member function is rejected when the overload set also has a non-static member` |

Attachments:

| File | Description |
|---|---|
| [`ctor-pre-static-overload.cpp`](ctor-pre-static-overload.cpp) | Constructor precondition and destructor postcondition calling a mixed static/non-static overload set |

````
An unqualified call in a constructor's precondition or a destructor's
postcondition is ill-formed only "if overload resolution selects a
non-static member function" ([over.call.func]/3).  When the overload set
mixes static and non-static member functions and resolution selects the
static one, the call is well-formed.  GCC diagnoses as soon as the set
contains a non-static member, before overload resolution.

```
struct S {
  bool mixed(int) const;
  static bool mixed(double);
  static bool sf(int);
  S(short) pre(sf(1));           // OK, accepted (control)
  S(char) pre(mixed(1.0));       // OK, selects the static overload
  ~S() post(mixed(1.0));         // OK, selects the static overload
};
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only ctor-pre-static-overload.cpp
ctor-pre-static-overload.cpp: In constructor 'S::S(char)':
ctor-pre-static-overload.cpp:16:15: error: 'S::mixed' 'this' required when accessing a member within a constructor precondition or destructor postcondition contract check
   16 |   S(char) pre(mixed(1.0));       // OK, selects the static overload
      |               ^~~~~
ctor-pre-static-overload.cpp: In destructor 'S::~S()':
ctor-pre-static-overload.cpp:17:13: error: 'S::mixed' 'this' required when accessing a member within a constructor precondition or destructor postcondition contract check
   17 |   ~S() post(mixed(1.0));         // OK, selects the static overload
      |             ^~~~~
```

Expected: no diagnostic, since mixed(double) is selected in both.  The
control, a call to a set containing only a static member, is accepted.


DISCOVERY

Found while reviewing an implementation of C++26 contracts against the
working draft.


ANALYSIS

From reading the code, not confirmed in a debugger: in
finish_id_expression_1 (gcc/cp/semantics.cc), the is_overloaded_fn branch
treats the set as members when !shared_member_p (decl) -- that is, when any
member of the set is non-static -- and the contract_class_ptr ==
current_class_ptr test then errors before the call is resolved.  For an
overload set the diagnostic belongs after overload resolution, when the
selected function is known to be non-static.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       rejected
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260922, f008f03eff8  yes
````

## Our Fix

`gcc/cp/semantics.cc`, `finish_id_expression_1`: for an overload set in a
constructor precondition or destructor postcondition that also has a static
member (`overload_set_has_static_member_p`), the call is built on a dummy
object, as in a static member function, so overload resolution decides:
selecting the static member is fine, selecting a non-static one is
diagnosed ("cannot call member function ... without object").  A set of
only non-static members keeps the existing diagnostic.  Regression test:
`g++.dg/contracts/cpp26/ctor-pre-static-overload.C`.

## Notes (ours, not for the report)

Fix direction: for an overload set, let the call be built and diagnose in
`build_new_method_call` / the implicit-object path when the selected
function is non-static.

Watch test: `g++.dg/contracts/cpp26/ctor-pre-static-overload.C`
(`dg-xfail-if`), mirrored in the Clang fork by
`clang/test/Contracts/OpenBugs/ctor-pre-static-overload.cpp` (XFAIL).
