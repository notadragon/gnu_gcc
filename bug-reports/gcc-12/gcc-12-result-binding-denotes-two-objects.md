# GCC-12: Postcondition result binding denotes two different objects within one predicate evaluation

**Status:** Fixed here (commit [3a9cc143cb14](https://github.com/notadragon/gnu_gcc/commit/3a9cc143cb14d9b9df1329d917dc8a587627fd2c))
**Resolved by:** `1130-result-binding-one-object`
**Component:** c++ / contracts
**Upstream Link:** [PR112794](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=112794)
**Affects:** stock g++ 16.2.0, trunk (17.0.0 20260901)

## Bug Report

Within a single evaluation of a single postcondition predicate, GCC's result
binding `r` resolves to two different objects depending on whether it is
accessed directly or through a reference parameter. The caller-visible
symptom is PR112794's lost/observed `const_cast` writes depending on
spelling -- the standard requires an evaluated predicate's effects to be
faithfully observable, and a result binding that aliases inconsistently
violates that.

## Reproducer

See [`gcc-12-result-binding-denotes-two-objects.cpp`](gcc-12-result-binding-denotes-two-objects.cpp)
in this directory.

## Our Fix

A gimplification fix making the result binding denote a single, consistent
object regardless of access spelling, per the standard's requirement that an
evaluated predicate's effects be faithfully observable.

## Notes

Matches upstream PR112794; our reproducer and fix can be linked as a comment
on that report.

## Pending upstream comment (PR112794)

*Drafted for the user to post or drop; nothing here has been sent.  Checked
2026-09-25: no comment from berne@notadragon.com on PR112794.  Every claim
re-measured 2026-09-25 against stock `17.0.0 20260922` (the Compiler
Explorer nightly) and 16.2.0, at -O0 and -O2.  This answers Dinka Ranns's
comment 1, which suggested [expr.call]/9 might allow the lost write.
Attach [3a9cc143cb14](https://github.com/notadragon/gnu_gcc/commit/3a9cc143cb14d9b9df1329d917dc8a587627fd2c) (the
`postcondition_needs_retval_temp_p` half) and the reproducer below.*

~~~text
On whether [expr.call]/9 makes this acceptable: a temporary for the return
value is permitted, but the result binding still has to denote one object
for a whole evaluation of the predicate.  GCC currently gives it two within
a single predicate, depending on how the sub-expression names it:

  #include <cstdio>
  const int *a_direct = nullptr, *a_viaref = nullptr;

  const int *addr_via_ref (const int &i) { return &i; }
  bool rec_direct (const int *p) { a_direct = p; return true; }
  bool rec_viaref (const int *p) { a_viaref = p; return true; }

  int f () post (r : rec_direct (&r) && rec_viaref (addr_via_ref (r)))
  { return 7; }

  int main () {
    f ();
    std::printf ("%p %p %s\n", (const void *) a_direct,
                 (const void *) a_viaref,
                 a_direct == a_viaref ? "same" : "different");
  }

prints two addresses 4 bytes apart ("different"), on 16.2.0 and on trunk
(17.0.0 20260922), at -O0 and -O2.  [dcl.contract.res]/1 lets the result
binding refer to the returned object or to a temporary, but either way to
one object, and [class.temporary] gives a choice of which object, not
permission for two.

The report's lost write is the same thing seen from the caller.  Three
spellings of one mutation, in one program:

  post (r : (const_cast<int&>(r) = 42, true))    caller sees 42
  post (r : mutate (r))  // mutate(const int&)   caller sees 0
  post (r : (const_cast<int&>(r)++, true))       caller sees 1

If a temporary had been introduced for the return value, that would be a
property of the function and all three would agree.  They do not, because
a scalar result is a gimple register and gimplify_addr_expr spills it to a
fresh temporary each time the predicate needs its address.

We have a fix: when the result is returned in registers, is trivially
copyable, and the result binding has its address taken, the postcondition
checks get one addressable temporary, initialized from DECL_RESULT and
copied back after the checks.  Every use in the predicate then names that
one object, the program above prints "same", and the three spellings give
42, 42 and 1.  The copy-back keeps the direct const_cast write that
g++.dg/contracts/cpp26/expr.prim.id.unqual.p7-4.C checks visible to the
caller.  The same change fixes the ICE in PR125574.
~~~
