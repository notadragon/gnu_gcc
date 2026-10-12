---
id: 0260-friend-contract-class-scope
subject: "c++: contracts: look a friend's predicate up in its class's scope"
depends: []
regenerates: []
fixes: [gcc-630]
---

## Rationale

GCC-630.  A friend declared in a class is in that class's scope, so an
unqualified name in its contract predicate is looked up there, and a friend
defined in the class is looked up as a member function definition is
([basic.lookup.unqual]).  The predicates of functions declared in a class
are parsed once the class is complete, and that late parse switched to the
function's `DECL_CONTEXT` -- for a friend, its namespace -- so a static data
member named unqualified was "not declared in this scope", whether the
friend was declared in the class or defined there.

The queue of deferred contract specifiers now records, with each function,
the class it was declared in, as the queues of default arguments and
noexcept-specifiers already do, and the late parse switches to that class.
The regression test covers a friend declared in the class and defined
outside it, one defined in the class, and a class template's friend, with a
member function as the control.

## Compile gap

None.  The queue and its late parse are upstream's.

## Contents

- gcc/cp/parser.h : /^  vec<cp_default_arg_entry, va_gc> \*contracts;$/
- gcc/cp/parser.cc : /^      FOR_EACH_VEC_SAFE_ELT \(unparsed_contracts, ix, e\)$/, /^\t  switch_to_class \(e->class_type\);$/, /^      vec_safe_push \(unparsed_contracts, entry\);$/
- gcc/testsuite/g++.dg/contracts/cpp26/friend-contract-class-scope.C : *
