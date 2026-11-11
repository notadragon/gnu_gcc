// One contract descriptor table per translation unit, shared by every
// contract check, and one violation data block per contract.  The dynamic
// selection sibling is contract-descriptor-table-per-tu-dynamic.C.
//
// Mirror: clang/test/Contracts/contract-descriptor-table-per-tu.cpp in
// the llvm_llvm-project fork (CLANG-659, where each check had emitted its
// own table).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

void f (int x) pre (x > 0) {}
void g (int x) pre (x > 1) {}
void h (int x) { contract_assert (x > 2); }

// { dg-final { scan-assembler-times {contract_desc_basic[0-9]*:} 1 } }
// { dg-final { scan-assembler-times {contract_desc_full[0-9]*:} 1 } }
// { dg-final { scan-assembler-times {contract_data[0-9]*:} 3 } }
