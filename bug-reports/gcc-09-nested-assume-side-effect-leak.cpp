// gcc-09-nested-assume-side-effect-leak.cpp                          -*-C++-*-
//
// GCC-9: nested [[assume]] leaks the inner one's side effects into the
// enclosing constant evaluation.
//
// (Numbered 9, not 8: TODO.md already calls the -std=c++29 libcontracts link
// gap "GCC-8", and that one is OURS and owes no upstream report, so reusing
// the number here would collide with it.)
//
//   g++ -std=c++23 gcc-09-nested-assume-side-effect-leak.cpp -fsyntax-only
//   -> static assertion failed: nested: modification leaked out of [[assume]]
//
// PLAIN C++23.  No contracts, no `-fcontracts', no P3850 anything.  This is
// upstream GCC's bug, found 2026-09-01 while reviewing the P3850 branch --
// the branch adds a second user of the machinery below and so reaches it far
// more easily, but does not cause it.
//
// PROVENANCE: MEASURED, not inferred.  Reproduces on stock g++ 14.4.0,
// 15.3.0 and 16.2.0 (three shipped releases) as well as on trunk; the faulty
// line is byte-identical in upstream/master and the P3850 branch does not
// touch it.  Clang is CORRECT here: stock clang++ 19.1.0 and 22.1.6, and our
// branch clang, all roll the modification back -- and note HOW: Clang declines
// to evaluate a side-effecting assumption at all, saying so under -Wassume
// ("assumption is ignored because it contains (potential) side-effects"),
// which sidesteps the whole question rather than solving it the way GCC does.
//
// MECHANISM.  The operand of [[assume]] is not evaluated, so when constant
// evaluation speculatively evaluates it anyway, `modifiable_tracker'
// (gcc/cp/constexpr.cc) keeps that invisible: while it is active,
// constexpr_global_ctx::get_value_ptr refuses stores to objects created
// outside the operand, and the destructor rolls back the ones it allowed.
//
// Trackers nest -- an [[assume]] whose operand calls a constexpr function
// containing another [[assume]] starts a second one -- but
// ~modifiable_tracker ends with
//
//     global->modifiable = nullptr;
//
// rather than restoring the ENCLOSING tracker's set.  So everything in the
// outer operand sequenced after the inner tracker is destroyed runs
// untracked: neither refused nor recorded, and therefore never rolled back.
//
// FIX: save constexpr_global_ctx::modifiable in the constructor and restore
// it in the destructor, exactly as the neighbouring fields already do.
//
// The same root cause has a second symptom on the P3850 branch, where the
// contract predicate is a second modifiable_tracker user:
// -Wcontract-constexpr-side-effect fires only where the tracker REFUSED a
// modification, so a predicate whose callee has its own contract assertion
// silently loses the warning.  That symptom needs contracts and so is not
// part of this report; see
// g++.dg/contracts/cpp26/contract-constexpr-side-effect-nested.C.

constexpr bool bump  (unsigned *p) { *p += 1; return true; }
constexpr bool inner (unsigned *p) { [[assume (*p < 100)]]; return true; }

// Control: a single tracker, and the modification is correctly rolled back.
constexpr unsigned
plain ()
{
  unsigned x = 0;
  [[assume (bump (&x))]];
  return x;
}

// The bug: inner's tracker is created and destroyed before bump runs, so
// bump's store escapes.
constexpr unsigned
nested ()
{
  unsigned x = 0;
  [[assume (inner (&x) && bump (&x))]];
  return x;
}

static_assert (plain  () == 0, "plain: modification leaked out of [[assume]]");
static_assert (nested () == 0, "nested: modification leaked out of [[assume]]");

int main () { }
