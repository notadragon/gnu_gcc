// The header is precompiled with a configuration file whose contents make
// every contract ignore; the contents of that same file then change to
// enforce before this TU is compiled with it.  The PCH records the parsed
// configuration (contract_config_hash), so it does not match and is not
// used: the header's configuration must not silently override the TU's.
// pch.exp removes the header source for the compile that uses the PCH, so
// the rejection is the error below; reading the header as source instead
// diagnoses the violated precondition in constant evaluation (checked by
// hand).  contract-config-rewritten.C rewrites the file with equivalent
// contents, and the PCH is used.
//
// The file is in the output directory, under one name.  Each dg-options
// string is evaluated as Tcl, so its [file copy] writes the file before each
// compilation: the header's copies contract-config-mismatch.json (ignore),
// this one contract-config-changed.json (enforce).
//
// Mirror: tests/tests/contracts/config/pch_file in the edgcpp_compiler fork
// (EDG-69); clang/test/Contracts/pch-contract-config-file-changed.cpp in the
// llvm_llvm-project fork, which covers this test and
// contract-config-rewritten.C in one.  Clang records no configuration in the
// PCH and applies the TU's, so there the changed contents are diagnosed in
// constant evaluation rather than the PCH being rejected.
// { dg-options "-std=c++26 -fcontracts -Winvalid-pch -fcontract-configuration-file=contract-config-changed.json[file copy -force ${srcdir}/g++.dg/pch/contract-config-changed.json contract-config-changed.json]" }

#include "contract-config-changed.H" // { dg-warning "contract configuration does not match" }
// { dg-error "PCH files were found, but they were invalid" "" { target *-*-* } .-1 }
// { dg-prune-output "No such file or directory" }
// { dg-prune-output "compilation terminated" }
// { dg-final { file delete contract-config-changed.json } }

constexpr int v = g (-1);
