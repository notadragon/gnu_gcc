// p3098-unexpanded-pack-capture.C with the templates instantiated: the
// template's diagnostic is the only one, where the instantiation used to ICE
// in tsubst_pack_expansion (GCC-638) or crash (GCC-639).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce" }

template <class... T>
int p3 (T... xs) post [xs] (true) { return 0; }		// { dg-error "not expanded" }
template <class... T>
int p5 (T... xs) post [...y = xs] (y > 0) { return 0; }	// { dg-error "not expanded" }

int main () { return p3 (1, 2) + p5 (1, 2); }
