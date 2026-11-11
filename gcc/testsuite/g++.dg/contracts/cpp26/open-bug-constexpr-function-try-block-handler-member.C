// Watch test for an open bug (GCC-755, stock GCC too; not a contracts bug):
// referring to a non-static member in the handler of a constructor's
// function-try-block is undefined ([except.handle]/10), but constant
// evaluation accepts it.  When it is fixed, the xfailed dg-error line
// reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-function-try-block-handler-member.cpp.
// { dg-do compile { target c++26 } }

struct S
{
  int x;
  constexpr S (bool b) try : x (1) { if (b) throw 0; }
  catch (...) { int y = x; (void) y; }
};
constexpr bool f () { try { S s (true); } catch (...) {} return true; }
static_assert (f ());	// { dg-error "non-constant condition" "GCC-755" { xfail *-*-* } }
