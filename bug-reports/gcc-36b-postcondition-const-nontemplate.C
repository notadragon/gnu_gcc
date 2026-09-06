// GCC-36, shape B: a NON-template function reports the same [dcl.contract.func]
// const-parameter diagnostic twice -- literally the identical message, at the
// identical location.
//
//   g++ -std=c++26 -fcontracts -fsyntax-only <this file>
//
//   :1:24: error: a value parameter used in a postcondition must be const
//   :1:12: note: parameter declared here
//   :1:24: error: a value parameter used in a postcondition must be const
//   :1:12: note: parameter declared here
//
// Stock GCC 16.2.0 and trunk only.  Our contracts-p3850 branch reports this
// once, at the contract rather than at the odr-use, having diverged in
// check_postcondition_odr_use_r; so this shape is upstream's alone and is here
// for the "reproduces on stock" criterion, not because it is broken for us.

int f (int v) post (r: v == r) { return v; }
