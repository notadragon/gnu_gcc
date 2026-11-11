// gcc-33b-member-call-on-result.cpp                                 -*-C++-*-
//
// GCC-33 (PR126006): the same defect as gcc-33-class-result-binding-by-
// reference.cpp, reached through a member call on an empty-class result
// binding -- the implicit object parameter is the reference it is bound to.
//
//   g++ -std=c++26 -fcontracts -c gcc-33b-member-call-on-result.cpp
//
// Stock g++ trunk: internal compiler error: in expand_expr_addr_expr_1, at
// expr.cc during RTL expand.  Give G a data member and it compiles.
//
// PLAIN -fcontracts.

template <typename> struct generator
{
  bool is_valid () const;
};

// THE BUG, as reduced in PR126006.
generator<int> val () post (g : g.is_valid ())
{
  generator<int> x = {};
  return x;
}

// The same without the template.
struct G { bool is_valid () const; };
G val2 () post (g : g.is_valid ()) { G x = {}; return x; }
