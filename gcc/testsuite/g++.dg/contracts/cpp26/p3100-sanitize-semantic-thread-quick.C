// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=thread -fsanitize-semantic=thread:quick_enforce -fsanitize-semantic-print" }
int main() { return 0; }
// P3100 Task 4.1: quick_enforce (silent terminate, realized in the routing
// runtime without ever calling the handler) is available for every routed check
// regardless of the check's can_trap capability -- thread has can_trap=false yet
// still offers quick_enforce.  Accepted without -fcontracts-p4298.
// { dg-regexp "thread: quick_enforce" }
