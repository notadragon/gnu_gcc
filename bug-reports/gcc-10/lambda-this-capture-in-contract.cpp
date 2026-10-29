// A contract predicate on a lambda that captures `this' reads the closure
// object as if it were the enclosing class.
//
//   g++ -std=c++26 lambda-this-capture-in-contract.cpp -lstdc++exp \
//       -o t && ./t; echo $?
//
//     -> 1   the predicate did not see m
//     -> 0   expected: the predicate sees the same m the body does
//
// Wrong code, not an ICE, and the symptom is nondeterministic if you only
// watch whether the check fires: the predicate ends up comparing against a
// stack address, so whether any given comparison holds varies between
// builds.  This reproducer records the value the predicate actually saw,
// which fails deterministically.
//
// The generated operator() reads
//
//   _1 = MEM[(struct S *)__closure].m;      // the predicate -- WRONG
//   _2 = __closure->__this; _3 = _2->m;     // the body      -- right
//
// so the predicate reinterprets the closure object as an S and reads
// whatever sits at offset 0.

static int seen = -1;

static bool probe(int observed) { seen = observed; return true; }

struct S {
    int m;

    int through_this()
    {
        // The predicate reads m through the captured `this'.
        auto l = [this]() pre(probe(m)) { return m; };
        return l();
    }
};

int main()
{
    S s{2};
    int body = s.through_this();
    // The body reads m correctly; the predicate should see the same value.
    return (body == 2 && seen == 2) ? 0 : 1;
}
