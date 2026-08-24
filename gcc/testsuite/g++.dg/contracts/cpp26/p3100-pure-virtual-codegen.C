// P3100: the pure-virtual vtable slot (ub:class.abstract.pure.virtual) is
// pointed at the contract terminus selected by the configuration where the
// vtable is emitted.  Here the config selects quick_enforce, so the abstract
// class's vtable holds __cxa_pure_virtual_quick in the pure virtual's slot.
// An out-of-line key function (the destructor) forces the vtable into this TU.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-pure-virtual-quick.json" }

struct Base
{
  virtual void f () = 0;
  virtual ~Base ();		// out-of-line key function: emits Base's vtable here
};
Base::~Base () {}

// { dg-final { scan-assembler "__cxa_pure_virtual_quick" } }
