// P3100: configuring only the .direct leaf of ub:class.abstract.pure.virtual
// (a call resolved statically in the class's own constructor or destructor,
// checked at the call) leaves the vtable slot at __cxa_pure_virtual.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual.direct:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct Base
{
  virtual void f () = 0;
  virtual ~Base ();		// out-of-line key function: emits Base's vtable here
};
Base::~Base () {}
// { dg-final { scan-assembler-not "__cxa_pure_virtual_quick" } }
// { dg-final { scan-assembler "__cxa_pure_virtual" } }
