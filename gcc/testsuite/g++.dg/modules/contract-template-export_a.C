// An exported function template, a member of an exported class template and
// a function template with a result-name postcondition, each with a contract:
// the contract specifiers are streamed through the CMI, so an importer that
// instantiates the template checks them (GCC-553).
// contract-template-export_b.C imports and runs them.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fmodules -fcontracts -fcontract-evaluation-semantic=observe" }

export module contract_template_export;
// { dg-module-cmi "contract_template_export" }

export template <class T> T g1 (T x) pre (x > 0) { return x; }
export template <class T> struct TS { T k (T x) pre (x > 0) { return x; } };
export template <class T> T g2 (T x) post (r: r > 0) { return x; }
export int nn (int x) pre (x > 0) { return x; }
