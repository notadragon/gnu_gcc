// Under -fcontract-checks-outlined, a contract predicate that mutates a
// by-value parameter, or the postcondition result binding, writes to a copy.
// Neither the function body nor the caller sees the mutation, so the
// observable behaviour of a conforming program depends on a codegen flag.
//
//   g++ -std=c++26 outlined-checks-lose-by-value-mutations.cpp \
//       -lstdc++exp -o t && ./t; echo $?
//
//     -> 0    inlined (the default)
//     -> 19   with -fcontract-checks-outlined  (16 | 2 | 1)
//
// The exit status is a bitmask so one run reports every entity separately.

int g = 0;

int by_value_parm(int n) pre(const_cast<int&>(n)++)
{
    return n;                  // required: 3.  Outlined gives 2.
}

int result_binding() post(r : const_cast<int&>(r)++)
{
    return 1;                  // caller sees 2 inlined, 1 outlined.
}

int ref_parm(int& n) pre(const_cast<int&>(n)++)
{
    return n;
}

void global_mut() pre(const_cast<int&>(g)++)
{
}

// The crux: one predicate that mutates both shared state and the by-value
// parameter.  Elision would drop both; this drops only one.
int g_log = 0;

bool bump(int& n)
{
    ++n;
    g_log = n;                 // shared state: propagates even when outlined
    return true;
}

int partial_application(int n) pre(bump(const_cast<int&>(n)))
{
    return n * 100 + g_log;    // faithful 303, fully elided 200, outlined 203
}

int main()
{
    int bad = 0;

    int r = partial_application(2);
    if (r != 303 && r != 200)
        bad |= 16;             // 203: neither faithful nor elided

    if (by_value_parm(2) != 3)
        bad |= 1;              // fires only when outlined

    if (result_binding() != 2)
        bad |= 2;              // fires only when outlined

    int v = 2;
    ref_parm(v);
    if (v != 3)
        bad |= 4;              // never fires

    g = 3;
    global_mut();
    if (g != 4)
        bad |= 8;              // never fires

    return bad;
}
