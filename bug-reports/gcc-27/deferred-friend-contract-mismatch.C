// A friend declaration's contract is never checked against an earlier
// declaration of the same function, whenever the friend is not that
// function's first declaration -- whether the earlier one is itself a
// friend, or an ordinary declaration.  Every other redeclaration path
// diagnoses a mismatch.
//
//   g++ -std=c++26 -c deferred-friend-contract-mismatch.C
//
//     -> no diagnostic for either case below.  Each second declaration
//        should draw "mismatched contract condition in declaration", as
//        the namespace-scope control in the report does.

// Case 1: two friend declarations of the same function.
struct C {
    friend int f(int x) pre(x > 0);
    friend int f(int x) pre(x < 0);   // accepted; should be a mismatch
};

int f(int x) { return x; }

// Case 2: an ordinary declaration, then a friend redeclaration with a
// different contract -- the friend need not come first, nor does the
// earlier declaration need to be a friend itself.
int h(int x) pre(x > 0);

struct D {
    friend int h(int x) pre(x < 0);   // accepted; should be a mismatch
};

int h(int x) { return x; }

int main() { return (f(1) == 1 && h(1) == 1) ? 0 : 1; }
