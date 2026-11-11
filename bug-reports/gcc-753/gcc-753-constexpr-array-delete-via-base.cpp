struct B { int i = 0; };
struct D : B {};                 // same size as B
constexpr bool f() {
  B* p = new D[2];
  delete[] p;                    // UB [expr.delete]/3
  return true;
}
static_assert(f());              // accepted; clang: "delete of pointer to subobject"
