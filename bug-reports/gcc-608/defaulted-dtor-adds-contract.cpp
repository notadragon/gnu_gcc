struct S { ~S (); };
S::~S () pre (true) = default;
