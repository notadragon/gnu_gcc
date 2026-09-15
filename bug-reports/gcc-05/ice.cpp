// The same defect as double-destroy.cpp, in the shape where it ICEs
// rather than miscompiling.
//
//   g++ -std=c++26 -c ice.cpp
//
//     -> internal compiler error: in gimple_add_tmp_var, at gimplify.cc:841
//
// -fsyntax-only alone compiles clean: the function has to be emitted.
//
// Each ingredient is necessary -- dropping any one of these compiles clean:
//
//   * the contract specifier (`pre' or `post'; the predicate is irrelevant)
//   * a return type with a non-trivial destructor
//   * returning a NAMED local (returning a prvalue, or a call result,
//     is clean)
//   * a declaration of any kind AFTER the first return statement
//     (`int other;' below -- moving it above the `if' is clean, and so is
//     deleting it)

struct D { ~D(); };

D f(int n) pre(true)
{
    D r;
    if (n)
        return r;
    int other;
    return r;
}
