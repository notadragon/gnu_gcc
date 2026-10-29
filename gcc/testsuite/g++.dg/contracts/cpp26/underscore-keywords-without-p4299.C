// _Pre / _Post / _ContractAssert are the C (P4299) spellings and are keywords
// only with -fcontracts-p4299: without it they are plain identifiers in C++
// with contracts on.  (With -fcontracts-p4299 they are available in C++:
// p4299-in-cxx.C.)
//
// Mirror: clang/test/Contracts/underscore-keywords-without-p4299.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

int _Pre = 1, _Post = 2, _ContractAssert = 3;
int g () { return _Pre + _Post + _ContractAssert; }
