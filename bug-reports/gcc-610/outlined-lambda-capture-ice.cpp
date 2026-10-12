// A lambda's explicit capture named in its contract, under
// -fcontract-checks-outlined: internal compiler error in
// gimplify_var_or_parm_decl.
//
//   g++ -std=c++26 -fcontracts -fcontract-checks-outlined -c outlined-lambda-capture-ice.cpp
int main ()
{
  int k = 5;
  auto lam = [k] (const int x) pre (x != k) { return x; };
  return lam (1) - 1;
}
