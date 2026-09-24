// A friend declaration's contract is not checked against an earlier
// declaration of the same function, whenever the friend is not that
// function's first declaration.  Every other redeclaration path diagnoses a
// mismatch.
//
// Two shapes reach the defect, and they are selected separately because a
// single whole-file measurement averages them into one word: on this branch
// case 1 is now diagnosed and case 2 is not, so a combined run reports
// "error" and hides the half that is still broken.
//
//   g++ -std=c++26 -fcontracts -c deferred-friend-contract-mismatch.C -DCASE=1
//   g++ -std=c++26 -fcontracts -c deferred-friend-contract-mismatch.C -DCASE=2
//
//     -> on stock, no diagnostic for either case.  Each second declaration
//        should draw "mismatched contract condition in declaration", as the
//        namespace-scope control in the report does.

#ifndef CASE
#error define CASE to 1 or 2
#endif

#if CASE == 1

// Case 1: two friend declarations of the same function.
struct C {
    friend int f(int x) pre(x > 0);
    friend int f(int x) pre(x < 0);   // should be a mismatch
};

int f(int x) { return x; }

int main() { return f(1) == 1 ? 0 : 1; }

#elif CASE == 2

// Case 2: an ordinary declaration, then a friend redeclaration with a
// different contract -- the friend need not come first, nor does the
// earlier declaration need to be a friend itself.
int h(int x) pre(x > 0);

struct D {
    friend int h(int x) pre(x < 0);   // should be a mismatch
};

int h(int x) { return x; }

int main() { return h(1) == 1 ? 0 : 1; }

#else
#error CASE must be 1 or 2
#endif
