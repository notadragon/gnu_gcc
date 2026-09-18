// N5008:
// dcl.contract.func/p6
// A virtual function shall not have a function-contract-specifier-seq
// (unless -fcontracts-p3097 is enabled).
// { dg-do compile { target c++23 } }
// { dg-additional-options "-fcontracts" }

struct Base
{
  virtual int f1() pre(true); // { dg-error "contracts cannot be added to virtual functions" }
  virtual int f2();

};
struct Child : Base
{
  int f2() pre(true); // { dg-error "contracts cannot be added to virtual functions" }

};
