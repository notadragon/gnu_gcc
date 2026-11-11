// P4451: "import std;" declares std::contracts::labels::terminating, so
// a `<terminating>' assertion-control-specifier after it is accepted.
// Under -fcontracts-p4451 alone.

// { dg-additional-options "-fmodules --compile-std-module -fcontracts-p4451" }
// { dg-do compile { target c++26 } }
// { dg-module-cmi std }
// { dg-module-cmi std.compat }
// { dg-module-cmi <bits/stdc++.h> }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

import std;

int f (int x) pre<terminating> (x > 0) post<terminating> (r : r > 0)
{
  contract_assert<terminating> (x > 1);
  return x;
}

struct S
{
  int m (int x) pre<terminating> (x > 0) { return x; }
};

constexpr auto &t = std::contracts::labels::terminating;
