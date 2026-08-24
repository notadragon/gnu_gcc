// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=address -fsanitize=pointer-subtract -fsanitize-semantic=pointer-subtract:noexcept_observe" }
int main() { return 0; }
// pointer-subtract:noexcept_observe is a valid request for the routed
// pointer-subtract check (expr.add.sub.diff.pointers UB) under
// -fcontracts-p4298 (Task 4.1) -- no diagnostic expected.  Plain throwing
// observe/enforce are NOT valid for a routed check.  (pointer-subtract must be
// combined with -fsanitize=address.)
