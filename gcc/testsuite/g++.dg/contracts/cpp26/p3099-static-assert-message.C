// P3099: the contract diagnostic-message grammar (a string literal, or a
// constant of class type with .size()/.data()) is the same grammar static_assert
// uses; verify both forms deliver the message in a static_assert diagnostic.
// Note the contrast with contracts on Clang, where a custom-type contract
// message ICEs (BUG-13) even though the static_assert form below works there --
// i.e. BUG-13 is specific to the contract message-parse path.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3099" }

struct Msg {
  constexpr int size() const { return 13; }
  constexpr const char* data() const { return "custom sa msg"; }
};

static_assert(sizeof(int) > 100, Msg{});  // { dg-error "custom sa msg" }
static_assert(sizeof(int) > 100, "plain literal sa msg");  // { dg-error "plain literal sa msg" }
