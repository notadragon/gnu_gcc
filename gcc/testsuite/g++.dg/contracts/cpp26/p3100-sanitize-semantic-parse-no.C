// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=address -fno-sanitize-semantic=address:observe" }
int main() { return 0; }
// The option is RejectNegative: the -fno- form must be refused, not
// silently treated as a positive request.
// { dg-error "unrecognized command-line option .-fno-sanitize-semantic=" "" { target *-*-* } 0 }
