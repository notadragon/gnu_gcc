// { dg-options "-std=c++26 -fcontracts -fcontract-configuration-file=contract-config-rewritten.json[file copy -force ${srcdir}/g++.dg/pch/contract-config-mismatch.json contract-config-rewritten.json]" }
constexpr int g (int x) pre (x > 0) { return x; }
