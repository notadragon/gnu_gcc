// Watch test for an open bug (GCC-660): a variable declared in a GNU
// statement expression in the postcondition of a generic lambda whose
// return type is deduced ICEs in gimplify_var_or_parm_decl when the
// operator() is instantiated: the emitted check names the variable, but
// its declaration is gone.  The predicate is substituted as if in a
// template while the return type is undeduced, then rebuilt once it is.
// With a trailing return type it compiles
// (stmt-expr-local-generic-lambda.C), as does a non-generic lambda
// (stmt-expr-local-lambda-deduced-post.C).  When it is fixed, dg-ice
// reports an XPASS.
//
// Mirror: clang/test/Contracts/stmt-expr-local-generic-lambda.cpp in the
// llvm_llvm-project fork covers the trailing-return-type shape.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }
// { dg-ice "GCC-660" }

int k (int v)
{
  auto m = [] (auto x) post (r: __extension__ ({ int z = r; z > 0; })) { return x; };
  return m (v);
}
