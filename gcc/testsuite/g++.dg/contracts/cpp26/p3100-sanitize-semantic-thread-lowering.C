// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=thread -fsanitize-semantic=thread:noexcept_observe -fsanitize-semantic-print" }
int main() { return 0; }
// P3100 Task 4.1: thread is a routed whole-program check; its routed allowed set
// is {assume, quick_enforce, noexcept_enforce, noexcept_observe}.  An explicit
// thread:noexcept_observe (continue after the handler) is accepted under
// -fcontracts-p4298 -- libtsan has no compile-time trap and no per-access
// recover/abort codegen variant, so continue-vs-terminate is decided in the
// runtime report leg; noexcept_observe is offered even though native thread has
// can_recover=false.  (-fsanitize-recover=thread cannot be used: GCC rejects it.)
// { dg-regexp "thread: noexcept_observe" }
