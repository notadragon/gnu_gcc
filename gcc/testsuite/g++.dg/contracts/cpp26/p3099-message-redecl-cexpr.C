// P3099: Verify redeclaration sameness is based on extracted text, not
// expression structure.  A constexpr message that produces different text
// on different lines (via source_location) must be diagnosed.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3099" }

#include <source_location>

struct LocMsg {
    char buf[64];

    constexpr LocMsg(std::source_location loc = std::source_location::current())
        : buf{} {
        int line = loc.line();
        buf[0] = 'L';
        buf[1] = '0' + (line / 10);
        buf[2] = '0' + (line % 10);
        buf[3] = '\0';
    }
    constexpr int size() const {
        int i = 0;
        while (buf[i]) ++i;
        return i;
    }
    constexpr const char* data() const { return buf; }
};

// Same expression, but different extracted text due to different lines.
void f(int x) pre(x > 0, LocMsg{});
void f(int x) pre(x > 0, LocMsg{});  // { dg-error "mismatched contract diagnostic message" }

// Same expression on separate lines, but same extracted text (no line dependency).
struct FixedMsg {
    constexpr int size() const { return 5; }
    constexpr const char* data() const { return "hello"; }
};

void g(int x) pre(x > 0, FixedMsg{});
void g(int x) pre(x > 0, FixedMsg{});  // OK, same text
