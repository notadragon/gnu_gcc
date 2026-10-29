// During construction of Bx, a dynamic_cast to Bx of a pointer to its
// virtual base V refers to the object under construction ([class.cdtor]/6)
// and yields `this`.  GCC evaluates it correctly.
//
// The mirror of CLANG-725 in the llvm_llvm-project fork (stock Clang too),
// where Clang's constant evaluation gives the wrong result; this pins
// GCC's behaviour on the same program.
// Mirror: llvm_llvm-project
// clang/test/Contracts/OpenBugs/constexpr-dynamic-cast-vbase-construction.cpp.
// { dg-do compile { target c++26 } }

struct V { virtual void f () {} };
struct Bx : virtual V
{
  bool good;
  constexpr Bx () { V *v = this; good = (dynamic_cast<Bx *> (v) == this); }
};
struct D : Bx { constexpr D () {} };
constexpr bool t1 () { D d; return d.good; }
static_assert (t1 ());
