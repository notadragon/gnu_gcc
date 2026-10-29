// P3100: the pure-virtual vtable slot is reached only by a dispatched call,
// the .indirect leaf of ub:class.abstract.pure.virtual; configuring that leaf
// selects the terminus.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual.indirect:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct Base
{
  virtual void f () = 0;
  virtual ~Base ();		// out-of-line key function: emits Base's vtable here
};
Base::~Base () {}
// { dg-final { scan-assembler "__cxa_pure_virtual_quick" } }
