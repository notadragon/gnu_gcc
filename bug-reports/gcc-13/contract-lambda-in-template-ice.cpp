// A lambda with a contract specifier whose predicate names the lambda's
// own parameter ICEs when the lambda is instantiated as part of a template.
//
//   g++ -std=c++26 -c \
//       contract-lambda-in-template-ice.cpp
//
//     -> internal compiler error: in expand_expr_real_1, at expr.cc:11648
//
// No nesting and no generic lambda is required, though this was first found
// inside a generic lambda.

template <class T>
int ft(T a)
{
    auto inner = [](int b) pre(b > 0) { return b; };
    return inner(a);
}

int f() { return ft(1); }

// Each ingredient is necessary.  Dropping any one of these compiles clean:
//
//   * the contract is a specifier on the lambda.  A contract_assert in the
//     lambda's body is part of the body, substitutes with it, and is fine:
//
//       auto inner = [](int b) { contract_assert(b > 0); return b; };
//
//   * the predicate names the lambda's own parameter.  pre(true), or a
//     predicate naming only a global, is fine.
//
//   * the lambda is instantiated as part of a template.  The same lambda in
//     a non-template function is fine, and so is one that is never called.
