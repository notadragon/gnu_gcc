// Under P3595 dynamic selection, one contract descriptor table per
// translation unit, and one violation data block per contract shared by
// every arm of the dispatch on the selector's result.
//
// Mirror: clang/test/Contracts/contract-descriptor-table-per-tu.cpp (DYN) in
// the llvm_llvm-project fork (CLANG-659, where each arm had emitted its own
// block).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-cxxname.json" }

namespace std::contracts {
  enum class evaluation_semantic : unsigned char {
    ignore = 1, observe, enforce, quick_enforce
  };
}

namespace mylib {
  std::contracts::evaluation_semantic contract_semantic ();
}

void f (int x) pre (x > 0) {}
void g (int x) pre (x > 1) {}

// { dg-final { scan-assembler-times {call[ \t]+_?_ZN5mylib17contract_semanticEv} 2 } }
// { dg-final { scan-assembler-times {contract_desc_basic[0-9]*:} 1 } }
// { dg-final { scan-assembler-times {contract_data[0-9]*:} 2 } }
