// PR c++/124486 -- the other half: what [dcl.contract.func]/6 must NOT reject.
// The diagnosed shapes are in pr124486.C.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097" }

// Defaulted, but NOT on its first declaration: the contract lives on the
// earlier declaration, which is where it belongs.
struct OutOfLine { OutOfLine () pre (true); int x = 0; };
OutOfLine::OutOfLine () = default;

// An ordinary function.
void ordinary (int x) pre (x > 0) { }

// P3097 lifts the paragraph's virtual-function case, and this branch
// implements P3097, so a contract on a virtual function must keep working.
// Deliberately not diagnosed; this pins that a future tightening cannot
// quietly take it back.
struct Base { virtual ~Base () = default; virtual int vf (int x) pre (x > 0); };
int Base::vf (int x) { return x; }
struct Derived : Base { int vf (int x) override pre (x > 0) { return x; } };

// A deleted or defaulted function with no contract is untouched.
struct PlainDeleted { void g () = delete; };
struct PlainDefaulted { PlainDefaulted () = default; int x = 0; };
