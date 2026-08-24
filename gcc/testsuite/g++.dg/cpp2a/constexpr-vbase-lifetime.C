// Mirror of clang/test/SemaCXX/constexpr-vbase-lifetime.cpp.
// A derived-to-virtual-base conversion (pointer or reference) consults the
// most-derived object's layout to compute the virtual-base offset, so it is a
// use of the object: performing it on an object outside its lifetime (deleted,
// or not yet constructed) is UB and not a constant expression.  A non-virtual
// base conversion uses a static offset and does not access the object.
// { dg-do compile { target c++26 } }

// A1: deleted glvalue -> virtual base.
namespace deleted_glvalue {
  struct B {};
  struct D : virtual B {};
  constexpr int f () {
    D *d = new D ();
    D &r = *d;
    delete d;
    B &b = r;			// { dg-error "use of allocated storage after deallocation" }
    (void) &b;
    return 0;
  }
  static_assert ((f (), true));	// { dg-error "non-constant condition for static assertion" }
}

// A2: deleted pointer -> virtual base.
namespace deleted_pointer {
  struct B {};
  struct D : virtual B {};
  constexpr int f () {
    D *p = new D ();
    delete p;
    B *b = p;			// { dg-error "use of allocated storage after deallocation" }
    (void) b;
    return 0;
  }
  static_assert ((f (), true));	// { dg-error "non-constant condition for static assertion" }
}

// A3: forming the address of a member of a not-yet-constructed virtual base.
// DIVERGENCE: Clang rejects this (the X -> W virtual-base conversion accesses
// x's dynamic type before x's construction has begun); GCC currently ACCEPTS
// it -- a pre-existing GCC under-diagnosis of the virtual-base
// member-address-before-construction case (the cdtor.before.ctor D4277R0
// conformance celink encodes exactly this Clang-rejects / GCC-accepts
// divergence).  Kept here to mirror the Clang test and document the divergence;
// GCC could be improved to reject it too.
namespace member_of_unconstructed_vbase {
  struct W { int j; };
  struct X : virtual W {};
  struct Y { int *p; X x; constexpr Y () : p (&x.j) {} };
  constexpr int f () { Y y; return y.p != nullptr; }
  static_assert ((f (), true));	// GCC accepts (see divergence note above)
}

// ---- Positive controls: these must remain constant expressions. ----

namespace live_object_ok {
  struct B {};
  struct D : virtual B {};
  constexpr int ok () {
    D d;
    B &b = d;
    (void) &b;
    D *p = new D ();
    B *q = p;
    (void) q;
    delete p;
    return 0;
  }
  static_assert ((ok (), true));
}

namespace during_construction_ok {
  struct B {};
  struct D : virtual B {
    B *made;
    // The virtual base is constructed before the derived-class body runs, so
    // the derived-to-virtual-base conversion here is valid.
    constexpr D () : made (this) {}
  };
  constexpr int ok () { D d; return d.made != nullptr; }
  static_assert ((ok (), true));
}

namespace nonvirtual_base_of_deleted_ok {
  struct B {};
  struct D : B {};		// non-virtual base: static offset, no object access
  constexpr int ok () {
    D *p = new D ();
    delete p;
    B *b = p;			// fine: no use of the object's layout
    (void) b;
    return 0;
  }
  static_assert ((ok (), true));
}
