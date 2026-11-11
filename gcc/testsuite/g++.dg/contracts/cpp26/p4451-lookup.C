// P4451 [basic.contract.control]: unqualified lookup of `terminating' from
// an assertion-control-specifier shall find nothing or only
// std::contracts::labels::terminating.  A parameter, a class member declared
// later (the specifier is a complete-class context), an enumerator, a
// namespace-scope variable, a block-scope variable, a lambda's parameter
// and an ambiguous lookup are rejected; a using-directive or
// using-declaration naming the standard one is not.  Under -fcontracts-p4451 alone.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p4451" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace n1
{
  void f (int terminating) pre<terminating> (true);	// { dg-error "unqualified lookup of .terminating. from this assertion-control-specifier finds" }
}

namespace n2
{
  struct S
  {
    void f () pre<terminating> (true);	// { dg-error "unqualified lookup of .terminating. from this assertion-control-specifier finds" }
    static constexpr int terminating = 0;
  };
}

namespace n3
{
  enum E { terminating };
  void f () pre<terminating> (true);	// { dg-error "unqualified lookup of .terminating. from this assertion-control-specifier finds" }
}

namespace n4
{
  constexpr int terminating = 0;
  void f () pre<terminating> (true);	// { dg-error "unqualified lookup of .terminating. from this assertion-control-specifier finds" }
}

namespace n5
{
  using namespace std::contracts::labels;
  void f () pre<terminating> (true);
}

namespace n6
{
  using std::contracts::labels::terminating;
  void f () pre<terminating> (true);
}

namespace n7
{
  namespace a { constexpr int terminating = 0; }
  namespace b { constexpr int terminating = 1; }
  using namespace a;
  using namespace b;
  void f () pre<terminating> (true);	// { dg-error "unqualified lookup of .terminating. from this assertion-control-specifier finds" }
}

namespace n8
{
  void g (int x)
  {
    int terminating = x;
    contract_assert<terminating> (x > 0);	// { dg-error "unqualified lookup of .terminating. from this assertion-control-specifier finds" }
  }
  auto l = [] (int terminating) pre<terminating> (true) { };	// { dg-error "unqualified lookup of .terminating. from this assertion-control-specifier finds" }
}
