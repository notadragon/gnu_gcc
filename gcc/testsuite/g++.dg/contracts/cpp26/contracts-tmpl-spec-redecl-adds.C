// An explicit specialization is a function of its own: a redeclaration of it
// cannot add contracts that its first declaration lacks ([dcl.contract.func]).
// Split from contracts-tmpl-spec2.C, which is a run test.
// { dg-do compile { target c++23 } }
// { dg-additional-options "-fcontracts" }

template<typename T>
int g2(T t) pre ( t > 0 )
{
  return (int)t;
}

template<>
int g2<double>(double t);

template<>
int g2<double>(double t) // { dg-error "declaration adds contracts" }
  pre ( t < 0 )
{
  return (int)t;
}
