// A hidden friend defined in a class template, with a precondition naming a
// member of the class: its contracts are substituted when its definition is
// instantiated, though regenerate_decl_from_template otherwise leaves a unique
// friend alone.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

template <class T> struct TS {
  friend void tg (TS s) pre (s.m > 0) {}
  T m;
};

int main () { tg (TS<int>{1}); }
