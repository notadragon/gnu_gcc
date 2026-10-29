// The operations on (*this) checked by ub:class.base.init.mem.fun, one of
// each leaf, for the p3100-base-init-mem-fun-split-*.C tests.
#include <typeinfo>
struct Base { Base (bool) { } };
struct D : Base {
  int f () const { return 7; }
  virtual int vf () const { return 7; }
  ~D () pre (this->f () == 7) post (this->f () == 7) { }
  D (int) : Base (f () == 7) { }			// .nonpolymorphic.mem.init
  D (long) pre (this->f () == 7) : Base (true) { }	// .nonpolymorphic.contract
  D (char) : Base (vf () == 7) { }			// .polymorphic.virtual.call
  D (short) : Base (&typeid (*this) != nullptr) { }	// .polymorphic.typeid
  D (bool) : Base (dynamic_cast<D *> (this) != nullptr) { }	// .polymorphic.dynamic.cast
};
D d1 (1), d2 (1L), d3 ('c'), d4 ((short) 1), d5 (true);
