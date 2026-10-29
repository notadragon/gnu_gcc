// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=address,undefined -fsanitize-semantic=address:noexcept_observe,undefined:noexcept_enforce" }
int main() { return 0; }
// A group name ("undefined") on the left of ':' must resolve every member
// check that takes the requested semantic -- no diagnostic expected.  Every
// member of "undefined" is routed, so it takes noexcept_enforce (valid under
// -fcontracts-p4298); the routed address check is set separately here to
// noexcept_observe.  p3100-sanitize-semantic-err-group-empty.C covers a
// semantic that no member takes.
