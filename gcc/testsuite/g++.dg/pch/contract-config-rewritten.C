// The header is precompiled with a configuration file that makes every
// contract ignore; that same file is then rewritten, laid out differently
// but with the same configuration, before this TU is compiled with it.  The
// PCH records the parsed configuration, not the file's text or time, so it
// matches and is used, and the precondition below is not evaluated.  The
// control for contract-config-changed.C, whose rewrite changes the
// configuration.
//
// Each dg-options string is evaluated as Tcl, so its [file copy] writes the
// file in the output directory before each compilation: the header's copies
// contract-config-mismatch.json, this one contract-config-rewritten.json.
//
// Mirror: clang/test/Contracts/pch-contract-config-file-changed.cpp in the
// llvm_llvm-project fork, which covers this test and contract-config-changed.C
// in one; Clang records no configuration in the PCH and applies the TU's, so
// there the PCH is used whatever the file's contents.
// { dg-options "-std=c++26 -fcontracts -Winvalid-pch -fcontract-configuration-file=contract-config-rewritten.json[file copy -force ${srcdir}/g++.dg/pch/contract-config-rewritten.json contract-config-rewritten.json]" }
// { dg-final { file delete contract-config-rewritten.json } }

#include "contract-config-rewritten.H"

constexpr int v = g (-1);
