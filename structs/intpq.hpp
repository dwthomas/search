#include <cassert>
#include <cstdlib>


template<class Elm> struct IntpqEntry {
	Elm *nxt, *prev;
	IntpqEntry(void) : nxt(NULL), prev(NULL) {}
};

template <class Ops, class Elm> class Intpq {
public:

	enum { Initsz = 64 };

	Intpq(unsigned int sz = Initsz)
		: fill(0), nresize(0), end(0), nbins(0), bins(NULL) {
		resize(sz);
	}

	~Intpq(void) {
		if (bins)
			free(bins);
	}

	void push(Elm *e, unsigned int prio) {
		if (prio >= nbins)
			resize(prio == 0 ? Initsz : (prio + 1) * 1.5);
		if (bins[prio]) {
			Ops::entry(bins[prio]).prev = e;
			Ops::entry(e).nxt = bins[prio];
		}
		bins[prio] = e;

		if (fill == 0 || prio < end)
			end = prio;

		fill++;
	}

	Elm *pop(void) {
		if (fill == 0)
			return NULL;

		for ( ; !bins[end]; end++)
			;
		assert(bins[end]);

		Elm *e = bins[end];
		IntpqEntry<Elm> &ent = Ops::entry(e);
		if (ent.nxt)
			Ops::entry(ent.nxt).prev = NULL;

		bins[end] = ent.nxt;
		ent.nxt = ent.prev = NULL;
		fill--;

		return e;
	}

	void rm(Elm *e, unsigned int prio) {
		IntpqEntry<Elm> &ent = Ops::entry(e);
		if (ent.nxt)
			Ops::entry(ent.nxt).prev = ent.prev;
		if (ent.prev)
			Ops::entry(ent.prev).nxt = ent.nxt;
		else
			bins[prio] = ent.nxt;
		ent.nxt = ent.prev = NULL;
		fill--;
	}

	bool empty(void) {
		return fill == 0;
	}

	bool mem(Elm *e) {
		return Ops::entry(e).nxt == NULL;
	}

private:

	void resize(unsigned int sz) {
		Elm **b = (Elm**) malloc(sz * sizeof(*b));

		for (unsigned int i = 0; i < nbins; i++)
			b[i] = bins[i];

		for (unsigned int i = nbins; i < sz; i++)
			b[i] = NULL;

		if (bins)
			free(bins);

		nbins = sz;
		bins = b;
		nresize++;
	}

	friend bool intpq_push_test(void);
	friend bool intpq_pop_test(void);

	unsigned long fill;
	unsigned int nresize, end, nbins;
	Elm **bins;
};