// GCC-56 / PR125587: a postcondition that READS its result name is rejected
// as non-constant during constant evaluation.  The result name is never
// bound to the return value, so the static potential-constant check on the
// predicate fails before the predicate is ever evaluated.
//
// Requires -fcontracts (C++26).  No library header is needed: the defect is
// in the front end's constant evaluator, not in <contracts>.

// (1) Upstream's own confirmed reduction, from PR125587 comment 1.
constexpr int
f (int i) post (res : res > 0)
{
  return i;
}

static constexpr int v = f (42);

// (2) The minimal form -- no parameter, and the result read is the only
//     thing the predicate does.
constexpr int
g () post (r : r == 1)
{
  return 1;
}

static_assert (g () == 1);

// (3) Control, and the reason this is a defect about the VALUE read rather
//     than about the name.  Here the result name is mentioned but never
//     evaluated, and every compiler measured accepts it.  If a future change
//     makes this line fail too, the defect has moved and the analysis in the
//     writeup no longer applies.
constexpr int
h (int i) post (res : sizeof (res) > 0)
{
  return i;
}

static constexpr int hv = h (42);
