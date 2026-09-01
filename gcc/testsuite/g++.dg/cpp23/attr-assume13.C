// Nested [[assume]] must not leak the inner one's side effects.
//
// The operand of [[assume]] is not evaluated, so when constant evaluation
// speculatively evaluates it anyway, modifiable_tracker exists to keep that
// invisible: it refuses stores to objects created outside the operand and
// rolls back the ones it allowed.
//
// Trackers nest -- an [[assume]] whose operand calls a constexpr function
// containing another [[assume]] starts a second one.  ~modifiable_tracker
// used to clear constexpr_global_ctx::modifiable outright instead of
// restoring the enclosing tracker's set, so everything in the outer operand
// sequenced after the inner assume ran untracked: neither refused nor
// recorded, and so never rolled back.  The modification then escaped into
// the enclosing constant evaluation.
//
// { dg-do compile { target c++23 } }

constexpr bool bump (unsigned *p) { *p += 1; return true; }
constexpr bool inner (unsigned *p) { [[assume (*p < 100)]]; return true; }

// Control: one tracker, the modification is rolled back.
constexpr unsigned
plain ()
{
  unsigned x = 0;
  [[assume (bump (&x))]];
  return x;
}

// The bug: inner's tracker is created and destroyed before bump runs.
constexpr unsigned
nested ()
{
  unsigned x = 0;
  [[assume (inner (&x) && bump (&x))]];
  return x;
}

// Three deep, and with the modification between the two inner assumes as
// well as after them.
constexpr unsigned
deep ()
{
  unsigned x = 0;
  [[assume (inner (&x) && bump (&x) && inner (&x) && bump (&x))]];
  return x;
}

// The enclosing tracker must still refuse, not merely roll back: an object
// created outside the operand may not be modified by it at all.
constexpr unsigned
after_inner ()
{
  unsigned x = 0;
  [[assume (inner (&x))]];
  [[assume (bump (&x))]];
  return x;
}

static_assert (plain () == 0, "plain");
static_assert (nested () == 0, "nested");
static_assert (deep () == 0, "deep");
static_assert (after_inner () == 0, "after_inner");
