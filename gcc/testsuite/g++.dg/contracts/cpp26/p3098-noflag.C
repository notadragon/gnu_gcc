// P3098: Postcondition captures require -fcontracts-p3098.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

void f(int i)
  post [i] (true); // { dg-error "postcondition captures require" }
