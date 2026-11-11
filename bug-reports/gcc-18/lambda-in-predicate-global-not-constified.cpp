// A variable named inside a lambda written within a contract predicate is
// not constified if it has namespace scope.
//
//   g++ -std=c++26 -fsyntax-only \
//       lambda-in-predicate-global-not-constified.cpp
//
//     -> only ++i is diagnosed.  ++n is accepted, and should not be.
//
// This is [expr.prim.id.unqual]/3's own example, the lambda half.  Each line
// below carries the outcome that paragraph requires.

int n = 0;

struct X { bool m(); };

struct Y {
    int z = 0;

    void f(int i, int* p, int& r, X x, X* px)
        pre([=, &i, *this] mutable {
            ++n;         // error expected: attempting to modify const lvalue
            ++i;         // error expected: attempting to modify const lvalue
            ++p;         // OK, refers to member of closure type
            ++r;         // OK, refers to non-reference member of closure type
            ++this->z;   // OK, captured *this
            ++z;         // OK, captured *this
            return true;
        }())
    {}
};
