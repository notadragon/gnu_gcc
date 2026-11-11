// P4405, DECISIONS.md E169 (14), E195: as
// p4405-analysis-dead-call-labels-O0.C at -O2.
// The rule (DECISIONS.md E169 (18)): an evaluation that is reached is
// always analyzed, here diagnosed; one never reached may be skipped.
// Shapes, as EDG-184's in the edgcpp_compiler fork
// (p4405/dead_call_label_cgen): a label inside a substatement of `if'
// that a constant condition never selects makes the code after it
// reachable (`l*': in the unselected `else', `l2', at a `case' and a
// `default' label, `l3' and `l4'), also inside a loop (`l7*', `l9') and in
// a template (`l8', `l9'); the code before the label is never reached
// (`l6', `l8').
// The branch a constant condition never selects, entered at a label,
// falls through to what follows the `if' (`b*', GCC-994).  A nested
// `switch''s case labels and a loop's `break' and `continue' make
// nothing reachable from outside (`s*').
// The front end skips what it skips at -O0; the proof shadow finds the
// rest of the code never reached, `s1' and `s4' included, and an
// evaluation never reached is proven.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }

#include <contracts>
using namespace std::contracts::labels;

void g (int x) pre<always_prove> (x != 0);

// Reached through a label.
void l1 (int c) { if (c) goto L; if (0) { L: g (0); } }	// { dg-error "contract predicate is always false" }
void l2 (int c) { if (c) goto L; if (1) {} else { L: g (0); } }	// { dg-error "contract predicate is always false" }
void l3 (int c) { switch (c) { case 0: if (0) { case 1: g (0); } } }	// { dg-error "contract predicate is always false" }
void l4 (int c) { switch (c) { case 0: if (0) { default: g (0); } } }	// { dg-error "contract predicate is always false" }
void l6 (int c)
{
  if (c)
    goto L;
  if (0)
    {
      g (0);	// { dg-bogus "(cannot be proven|always false)" }
    L:
      g (0);	// { dg-error "contract predicate is always false" }
    }
}
void l7 (int c) { if (c) goto L; if (0) { while (c) { L: g (0); } } }	// { dg-error "contract predicate is always false" }
void l7b (int c) { if (c) goto L; if (0) { while (c-- > 1) { L:; } g (0); } }	// { dg-error "contract predicate is always false" }
void l7c (int c) { if (c) goto L; if (0) { do { L: g (0); } while (0); } }	// { dg-error "contract predicate is always false" }
template <int N> void l8 (int c)
{
  if (c)
    goto L;
  if (N)
    {
      g (0);	// { dg-bogus "(cannot be proven|always false)" }
    L:
      g (0);	// { dg-error "contract predicate is always false" }
    }
}
template void l8<0> (int);
template <int N> void l9 (int c) { if (c) goto L; if (N) { while (c) { L: g (0); } } }	// { dg-error "contract predicate is always false" }
template void l9<0> (int);
// GCC-994: the branch a constant condition never selects, entered at a
// label, falls through to what follows the `if'.
void b1 (int c) { if (c) goto L; if (1) return; else { L:; } g (0); }	// { dg-error "contract predicate is always false" }
void b2 (int c) { if (c) goto L; if (0) { L:; } else return; g (0); }	// { dg-error "contract predicate is always false" }
void b3 (int c) { switch (c) { case 0: if (1) return; else { case 1:; } g (0); } }	// { dg-error "contract predicate is always false" }
void b4 (int c) { if (c) goto L; if (1) { if (1) return; else { L:; } } g (0); }	// { dg-error "contract predicate is always false" }
void b5 (int c) { if (c) goto L; if (0) { L:; } else throw 1; g (0); }	// { dg-error "contract predicate is always false" }
template <int N> void b6 (int c) { if (c) goto L; if (N) return; else { L:; } g (0); }	// { dg-error "contract predicate is always false" }
template void b6<1> (int);

// Never reached.
void s1 (int c) { if (0) { switch (c) { case 1: g (0); default: break; } } }	// { dg-bogus "(cannot be proven|always false)" }
void s2 (int c) { if (0) { while (c) { if (c > 1) break; g (0); } } }	// { dg-bogus "(cannot be proven|always false)" }
void s3 (int c) { if (0) { for (;;) { if (c) continue; g (0); } } }	// { dg-bogus "(cannot be proven|always false)" }
void s4 (int c) { switch (c) { case 0: if (0) { switch (c) { case 1: g (0); } } } }	// { dg-bogus "(cannot be proven|always false)" }
void s5 (int c) { if (1) return; else { switch (c) { case 1:; } } g (0); }	// { dg-bogus "(cannot be proven|always false)" }
void s6 (int c) { if (1) return; else {} g (0); }	// { dg-bogus "(cannot be proven|always false)" }
