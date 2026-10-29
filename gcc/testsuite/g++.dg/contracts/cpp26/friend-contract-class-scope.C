// A friend function's contract predicate finds members of the class it is
// declared in: a friend declared in a class is in that class's scope, so
// `limit' below is found ([basic.lookup.unqual]), for a friend declared in
// the class and for one defined there (GCC-630).  The member function is
// the control; a class template's friend is checked too.
//
// Mirror: clang/test/Contracts/friend-contract-class-scope.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

struct S
{
  static constexpr int limit = 10;
  int m (int x) pre (x != limit) { return x; }
  friend int declared (int x) pre (x != limit); // { dg-bogus "not declared" }
  friend int defined (int x) pre (x != limit) { return x; } // { dg-bogus "not declared" }
};

int declared (int x) { return x; }

template <class T>
struct U
{
  static constexpr T limit = 10;
  friend T tdefined (U, T x) pre (x != limit) { return x; } // { dg-bogus "not declared" }
};

U<int> u;
int a = declared (1) + tdefined (u, 3);
