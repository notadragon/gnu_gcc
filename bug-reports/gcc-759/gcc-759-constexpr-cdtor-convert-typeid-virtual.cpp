struct A { int tag = 1; };
struct CBase : virtual A {};
struct DBase : virtual A { A* p; constexpr DBase(A* a) : p(a) {} };
struct E : CBase, DBase {
  constexpr E() : DBase(this) {}   // UB [class.cdtor]/3: DBase not yet constructed
};
constexpr int t() { E e; return e.p == static_cast<A*>(&e); }
static_assert(t());
