// GCC-630: a friend function's contract predicate does not find members of
// the class it is declared in.  Compile with -std=c++26 -fcontracts.
//
// A friend declared in a class is in the scope of that class: names in its
// declarator, including its function contract specifiers, are looked up
// there, and a friend defined in the class is looked up as a member function
// definition is ([basic.lookup.unqual]).  So each predicate below finds
// S::limit.  Clang (with contracts) and EDG accept all of it.

struct S
{
  static constexpr int limit = 10;

  // CONTROL: a member function's predicate finds it.
  int m (int x) pre (x != limit) { return x; }

  // THE BUG: a friend declared here, defined below.
  friend int declared (int x) pre (x != limit);

  // THE BUG: a friend defined here.
  friend int defined (int x) pre (x != limit) { return x; }
};

int declared (int x) { return x; }
