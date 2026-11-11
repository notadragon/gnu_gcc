// { dg-options "-std=c++26 -fcontracts -fcontract-configuration-file=${srcdir}/g++.dg/pch/contract-config-mismatch.json" }
constexpr int g (int x) pre (x > 0) { return x; }
