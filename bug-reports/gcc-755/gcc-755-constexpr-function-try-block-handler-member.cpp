struct S {
  int x;
  constexpr S(bool b) try : x(1) { if (b) throw 0; }
  catch (...) { int y = x; (void)y; }   // UB [except.handle]/10
};
constexpr bool f() { try { S s(true); } catch (...) {} return true; }
static_assert(f());
