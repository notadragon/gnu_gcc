// Second translation unit for p3400-rethrow-bypass-weak-callee.C:
// the strong definition that replaces the weak rethrow_helper at link time.

extern int logged;
[[noreturn]] void rethrow_helper () { ++logged; throw; }
