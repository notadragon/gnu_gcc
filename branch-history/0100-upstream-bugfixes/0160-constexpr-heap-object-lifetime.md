---
id: 0160-constexpr-heap-object-lifetime
subject: 'c++: end the lifetime of a destroyed heap object in constant evaluation'
depends: []
regenerates: []
fixes: [gcc-69, gcc-70]
---

## Rationale

Two ways constant evaluation accepts undefined operations on an object
created by a new-expression, both still open upstream.

PR126425 (GCC-70).  A non-array new-expression's object is element 0 of its
heap variable, which the evaluator types as a one-element array.  When the
object's destructor ends its lifetime, the end-of-lifetime clobber therefore
names a subobject of the variable, and the store code only resets that
subobject's value: the variable never becomes "outside its lifetime", as a
declared object does.  Destroying the object again -- a second destructor
call, or a delete-expression after an explicit destructor call -- and
reading or writing it were all accepted.  A clobber that ends the lifetime of
exactly that element now ends the variable's lifetime, with its storage
still allocated, so the existing outside-lifetime diagnostics apply;
constructing a new object in the storage starts a lifetime again, and the
storage can still be deallocated.  The diagnostics name the object as an
allocated one rather than printing the artificial variable.

PR97647 (GCC-69).  Deleting an object through a pointer to a base class
whose destructor is not virtual is undefined ([expr.delete]/3).  The
replaceable deallocation branch now compares the delete operand's static
class type, visible just inside its conversion to `void *`, with the heap
object's type.  A virtual destructor's deleting destructor passes the
complete object, so that case is unaffected.  Two existing tests,
`cpp2a/constexpr-new11.C` and `cpp2a/constexpr-new12.C`, deleted a `B`
through an `A *` whose destructor was not virtual -- the undefined
operation this now diagnoses -- and they gain a virtual destructor, which is
what their tests of virtual calls and use after deletion need.

The lifetime rule applies only to a one-element heap variable: storage an
allocator obtains for several objects is an array of several elements,
each with a lifetime of its own.

## Compile gap

None: plain C++20 constant evaluation, no contracts.  The tests sit under
`g++.dg/contracts/cpp26/` because that is where the watch tests that found
the defects lived.

## Contents

- gcc/cp/constexpr.cc : /A non-array new-expression's object is element 0 of its heap/, /allocated object outside its/, /The operand's static type, just inside its conversion/, /delete of an object of dynamic/
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-delete-nonvirtual-dtor.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-heap-double-destroy.C : *
- gcc/testsuite/g++.dg/cpp2a/constexpr-new11.C : *
- gcc/testsuite/g++.dg/cpp2a/constexpr-new12.C : *
