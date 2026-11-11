// Consumer for contract-control-3_a.X: the contract_control directive from
// "import <contracts>;" is active inside an assertion-control expression
// and inactive everywhere else, including a contract predicate.
// { dg-additional-options "-fmodules -fcontracts-p3400" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

import <contracts>;

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
