// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=address -fsanitize-semantic=address:noexcept_observe" }
int main() { return 0; }
// address:noexcept_observe is a valid request for the routed "address" check
// under -fcontracts-p4298 (Task 4.1) -- no diagnostic expected.  Plain
// throwing observe/enforce are NOT valid for a routed check; see
// p3100-sanitize-semantic-err-address-observe.C.
