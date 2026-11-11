// P3400: the contract_control directive "import std;" exports
// (std-contract-control_a.C) is active inside an assertion-control
// expression and inactive everywhere else, including a contract predicate.

// { dg-additional-options "-fmodules --compile-std-module -fcontracts-p3400" }
// { dg-do compile { target c++26 } }
// { dg-module-cmi std }
// { dg-module-cmi std.compat }
// { dg-module-cmi <bits/stdc++.h> }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

import std;

// Active: an assertion-control expression.
int f (int x) pre<review> (x > 0) pre<terminating> (x > 1)
{
  return x;
}

// Inactive: ordinary lookup, at namespace scope, in a function body and in
// a predicate.
auto leak1 = review;  // { dg-error "'review' was not declared in this scope" }
auto leak2 = terminating;  // { dg-error "'terminating' was not declared in this scope" }

void
g ()
{
  auto leak3 = review;  // { dg-error "'review' was not declared in this scope" }
}

int h (int x) pre (&review != nullptr);  // { dg-error "'review' was not declared in this scope" }
