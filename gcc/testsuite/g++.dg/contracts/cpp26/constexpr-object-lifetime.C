// Constant evaluation rejects destroying an object twice (an explicit
// destructor call then delete or the end of its scope; a trivial destructor
// called twice) and writing to an object after its destruction, and accepts
// reconstructing a destroyed object with std::construct_at, a store that
// starts a union member's lifetime, and a destroyed subobject or array
// element reconstructed ([basic.life]).  Not contracts-specific; from the EDG
// fork's sema/constexpr_object_lifetime (EDG-44).
//
// Mirror: clang/test/Contracts/constexpr-object-lifetime.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }

#include <memory>
struct S { float f = 0; constexpr ~S() {} };  // { dg-error "destroying (an allocated object|.s.) outside its lifetime" }
constexpr bool destroy_then_delete() { S *p = new S; p->~S(); delete p; return true; }
static_assert(destroy_then_delete());           // { dg-error "non-constant condition" }
constexpr bool write_after_destroy() {
  S *p = new S; p->~S(); p->f = 1; delete p;  // { dg-error "modification of an allocated object outside its lifetime" }
  return true;
}
static_assert(write_after_destroy());           // { dg-error "non-constant condition" }
constexpr bool local_twice() { S s; s.~S(); return true; }
static_assert(local_twice());                   // { dg-error "non-constant condition" }
struct T { int i = 0; };
constexpr bool trivial_twice() { T t; t.~T(); t.~T(); return true; }  // { dg-error "destroying .t. outside its lifetime" }
static_assert(trivial_twice());                 // { dg-error "non-constant condition" }
constexpr bool assign_after_destroy() {
  T *p = new T; p->~T(); *p = T{}; delete p;  // { dg-error "modification of an allocated object outside its lifetime" }
  return true;
}
static_assert(assign_after_destroy());          // { dg-error "non-constant condition" }
constexpr bool explicit_twice() {
  S *p = new S; p->~S(); p->~S(); return true;
}
static_assert(explicit_twice());                // { dg-error "non-constant condition" }
constexpr bool write_after_local_destroy() {
  S s; s.~S(); s.f = 1; return true;  // { dg-error "modification of .s. outside its lifetime" }
}
static_assert(write_after_local_destroy());     // { dg-error "non-constant condition" }

constexpr bool reconstruct() {
  S s;
  s.~S();
  std::construct_at(&s);
  s.f = 2;
  return s.f == 2;                              // OK, destroyed again at '}'
}
static_assert(reconstruct());
constexpr bool reconstruct_heap() {
  S *p = new S;
  p->~S();
  std::construct_at(p);
  delete p;                                     // OK
  return true;
}
static_assert(reconstruct_heap());
union U { int i; float f; };
constexpr bool union_member() {
  U u{1};
  u.f = 2;                                      // OK, starts f's lifetime
  return u.f == 2;
}
static_assert(union_member());
constexpr bool member_destroyed() {
  struct P { S a, b; };
  P p;
  p.a.~S();                                     // A subobject: not tracked
  std::construct_at(&p.a);
  return true;
}
static_assert(member_destroyed());
constexpr bool array_elements() {
  S *p = new S[2];
  p[0].~S();
  std::construct_at(&p[0]);
  delete[] p;
  return true;
}
static_assert(array_elements());
