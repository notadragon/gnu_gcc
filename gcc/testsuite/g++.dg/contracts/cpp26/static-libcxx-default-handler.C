// With -static-libstdc++, a violation reaching the default handler is reported
// by libstdc++'s default handler (libstdc++exp is always static).  (Clang with
// a static libc++ uses libcontracts' fallback printer instead, which is
// accepted: DECISIONS.md M3.)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe -static-libstdc++" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int f (int x) pre (x > 0) { return x; }

int main () { f (-1); }

// { dg-output "\\\[assertion_kind: pre, semantic: observe, mode: predicate_false" }
