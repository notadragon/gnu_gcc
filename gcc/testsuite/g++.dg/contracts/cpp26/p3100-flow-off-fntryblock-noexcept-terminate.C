// P3100: a NOEXCEPT function-try-block whose handler runs off its own end.
// The try body falls off -> inside check throws -> caught by the handler ->
// handler falls off -> after-construct check throws -> the exception escapes
// the noexcept boundary -> std::terminate.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-flow-off-fntryblock-observe.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "handler runs off its end in a noexcept function -> terminate" }

#include <contracts>

struct E {};
void handle_contract_violation (const std::contracts::contract_violation&) {
  throw E{};
}

int f (int x) noexcept try { if (x > 0) return x; } catch (E&) { /* no return */ }

int main () { f (-1); }
