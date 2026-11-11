// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=shift -fno-sanitize-recover=shift -fsanitize-semantic-print" }
int main() { return 0; }
// shift is a routed UBSan check (full-coverage routing), so a terminating
// request without -fcontracts-p4298 lowers to quick_enforce (a throwing
// enforce cannot propagate from a routed sanitizer's report, so without the
// nonthrowing-semantics flag the only available terminating semantic is the
// handler-less quick_enforce).  -fno-sanitize-recover=shift selects the
// terminating direction (-fsanitize=undefined's suboptions default to recovery
// on, so it must be requested explicitly).  With -fcontracts-p4298 this would
// instead lower to noexcept_enforce.
// { dg-regexp "shift: quick_enforce" }
