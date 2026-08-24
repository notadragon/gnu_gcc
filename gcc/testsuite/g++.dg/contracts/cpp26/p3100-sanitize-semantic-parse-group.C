// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=address,undefined -fsanitize-semantic=address:noexcept_observe,undefined:enforce" }
int main() { return 0; }
// A group name ("undefined") on the left of ':' must resolve every member
// check to the requested semantic -- no diagnostic expected.  The "undefined"
// group does not include the routed "address" check, so undefined:enforce is
// unaffected by Task 4.1; the routed address check is set separately here to
// noexcept_observe (valid under -fcontracts-p4298).
