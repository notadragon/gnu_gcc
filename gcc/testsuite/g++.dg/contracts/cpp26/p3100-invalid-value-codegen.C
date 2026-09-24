// P3100: an invalid bool/enum value load is instrumented per site.  Under
// "assume" (no configuration) the load is a plain load (byte-identical, the
// value is assumed in range).  Under "ignore" the storage bits are reloaded and
// an out-of-range value is replaced by a defined valid value via a select
// (`out_of_range ? 0 : raw`) -- see instrument_bool_enum_load_contract.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O2 -fdump-tree-ubsan -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-invalid-value-codegen.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

bool a_load (const bool *p) { return *p; }             // assume (no config)

namespace ign_ns {
  bool load (const bool *p) { return *p; }             // ignore -> instrumented
}

// Exactly one instrumented load: the ign_ns one gets the value-substituting
// select; a_load stays a plain load.
// { dg-final { scan-tree-dump-times "\\? 0 :" 1 "ubsan" } }
