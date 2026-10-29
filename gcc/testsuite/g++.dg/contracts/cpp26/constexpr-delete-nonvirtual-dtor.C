// Deleting an object through a pointer to a base class whose destructor is
// not virtual is undefined ([expr.delete]/3), so it is not a core constant
// expression ([expr.const]).  Constant evaluation used to accept it and run
// only the base destructor (GCC-69, upstream PR97647).  With a virtual
// destructor the same delete is fine.
//
// Mirror: clang/test/Contracts/constexpr-delete-nonvirtual-dtor.cpp.
// { dg-do compile { target c++20 } }

struct B { int a; constexpr ~B () {} };
struct D : B { int b; constexpr ~D () {} };

constexpr bool
f ()
{
  B *b = new D;
  delete b; // { dg-error "delete of an object of dynamic type 'D' through a pointer to 'B'" }
  return true;
}

static_assert (f ()); // { dg-error "non-constant condition" }

// Same size: the check does not rely on the sized deallocation's size.
struct E : B { constexpr ~E () {} };

constexpr bool
g ()
{
  B *b = new E;
  delete b; // { dg-error "delete of an object of dynamic type 'E' through a pointer to 'B'" }
  return true;
}

static_assert (g ()); // { dg-error "non-constant condition" }

struct VB { int a; constexpr virtual ~VB () {} };
struct VD : VB { int b; constexpr ~VD () override {} };

constexpr bool
h ()
{
  VB *b = new VD;
  delete b;
  D *d = new D;
  delete d;
  return true;
}

static_assert (h ());
