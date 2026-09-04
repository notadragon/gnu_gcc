// GCC-20 / PR126804, PR126881: a variadic parameter pack of non-reference
// type, named in a postcondition through a fold-expression, ICEs a
// constexpr-eligible function template's substitution.
//
// Freshly constructed (NOT extracted from a test) -- see the parent .md's
// Notes for why. Reduced from the shape in gnu_gcc commit 3eb1e3f1974's
// log message (a pack-index-expression example) after that literal
// example did not reproduce; this fold-expression shape does. Requires
// -std=c++26 -fcontracts. PR126881 notes -O1 is needed for the defect to
// manifest as wrong code rather than "just" an ICE; this reproducer ICEs
// with or without -O1 on the stock compiler checked, which is a stronger
// (and simpler) demonstration of the same underlying defect.
//
// ICEs the compiler's own substitution machinery ("internal compiler
// error: in tsubst, at cp/pt.cc:...") on stock g++ 16.2.0, with or without
// -O1. Compiles cleanly on g++ trunk (17.0.0 20260901). This exact
// reproducer was not additionally verified against this branch's own
// installed compiler (none was available in this session); the fix
// commit's own test suite run (2933 pass / 4 XFAIL / 6 unsupported / 0
// unexpected, per its log message) is the evidence that this branch does
// not regress on pack-in-contract shapes generally.

template <typename... Ts>
bool f (const Ts... a) post (r : (... && (a > 0)) == r)
{
  return (... && (a > 0));
}

int
main ()
{
  return f (1, 2, 3) ? 0 : 1;
}
