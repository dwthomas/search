// Best-first Utility-Guided Search Yes! From:
// "Best-first Utility-Guided Search," Wheeler Ruml and Minh B. Do,
// Proceedings of the Twentieth International Joint Conference on
// Artificial Intelligence (IJCAI-07), 2007

#include "../search/search.hpp"
#include "../structs/binheap.hpp"
#include "../utils/pool.hpp"

template <class D> struct Bugsy : public SearchAlgorithm<D> {
	typedef typename D::State State;
	typedef typename D::PackedState PackedState;
	typedef typename D::Cost Cost;
	typedef typename D::Oper Oper;

	struct Node : SearchNode<D> {
		typename D::Cost f, h, d;
		double u, t;

		static bool pred(Node *a, Node *b) {
			if (a->u != b->u)
				return a->u > b->u;
			if (a->t != b->t)
				return a->t < b->t;
			if (a->f != b->f)
				return a->f < b->f;
			return a->g > b->g;
		}
	};

	Bugsy(int argc, const char *argv[]) :
			SearchAlgorithm<D>(argc, argv),
			navgh(0), navgd(0), herror(0), derror(0),
			timeper(0.0), nresort(0), batchsize(20), nexp(0), state(WaitTick),
			closed(30000001) {
		wf = wt = -1;
		for (int i = 0; i < argc; i++) {
			if (i < argc - 1 && strcmp(argv[i], "-wf") == 0)
				wf = strtod(argv[++i], NULL);
			else if (i < argc - 1 && strcmp(argv[i], "-wt") == 0)
				wt = strtod(argv[++i], NULL);
		}

		if (wf < 0)
			fatal("Must specify non-negative f-weight using -wf");
		if (wt < 0)
			fatal("Must specify non-negative t-weight using -wt");

		nodes = new Pool<Node>();
	}

	~Bugsy(void) {
		delete nodes;
	}

	enum { WaitTick, ExpandSome, WaitExpand };

	void search(D &d, typename D::State &s0) {
		this->start();
		closed.init(d);
		Node *n0 = init(d, s0);
		closed.add(n0);
		open.push(n0);

		lasttick = walltime();
		while (!open.empty() && !SearchAlgorithm<D>::limit()) {
			updatetime();

			Node* n = *open.pop();
			State buf, &state = d.unpack(buf, n->packed);
			if (d.isgoal(state)) {
				SearchAlgorithm<D>::res.goal(d, n);
				break;
			}
			expand(d, n, state);
		}

		this->finish();
	}

	virtual void reset(void) {
		SearchAlgorithm<D>::reset();
		open.clear();
		closed.clear();
		delete nodes;
		timeper = 0.0;
		state = WaitTick;
		batchsize = 20;
		nexp = 0;
		nresort = 0;
		navgh = navgd = 0;
		herror = derror = 0;
		nodes = new Pool<Node>();
	}

	virtual void output(FILE *out) {
		SearchAlgorithm<D>::output(out);
		closed.prstats(stdout, "closed ");
		dfpair(stdout, "open list type", "%s", "binary heap");
		dfpair(stdout, "node size", "%u", sizeof(Node));
		dfpair(stdout, "cost weight", "%g", wf);
		dfpair(stdout, "time weight", "%g", wt);
		dfpair(stdout, "final time per expand", "%g", timeper);
		dfpair(stdout, "number of resorts", "%lu", nresort);
		dfpair(stdout, "mean single-step h error", "%g", herror);
		dfpair(stdout, "mean single-step d error", "%g", derror);
	}

private:

	// Kidinfo holds information about a node used for
	// correcting the heuristic estimates.
	struct Kidinfo {
		Kidinfo(void) : f(-1), h(-1), d(-1) { }

		Kidinfo(Cost g, Cost _h, Cost _d) : f(g + _h), h(_h), d(_d) { }

		Cost f, h, d;
	};

	// expand expands the node, adding its children to the
	// open and closed lists as appropriate.
	void expand(D &d, Node *n, State &state) {
		this->res.expd++;
		Kidinfo bestinfo;

		typename D::Operators ops(d, state);
		for (unsigned int i = 0; i < ops.size(); i++) {
			if (ops[i] == n->pop)
				continue;

			this->res.gend++;
			Kidinfo kinfo = considerkid(d, n, state, ops[i]);
			if (bestinfo.f < Cost(0) || kinfo.f < bestinfo.f)
				bestinfo = kinfo;
		}

		if (bestinfo.f < Cost(0))
			return;

		double herr = bestinfo.f - n->f;
		if (herr >= 0) {
			navgh++;
			herror = herror + (herr - herror)/navgh;
		}
		double derr = bestinfo.d + 1 - n->d;
		if (derr >= 0) {
			navgd++;
			derror = derror + (derr - derror)/navgd;
		}
	}

	// considers adding the child generated via the given
	// operator to the open and closed lists.  The return is
	// the info struct for the generated child.
	Kidinfo considerkid(D &d, Node *parent, State &state, Oper op) {
		Node *kid = nodes->construct();
		typename D::Edge e(d, state, op);
		kid->g = parent->g + e.cost;
		d.pack(kid->packed, e.state);

		unsigned long hash = d.hash(kid->packed);
		Node *dup = static_cast<Node*>(closed.find(kid->packed, hash));
		if (dup) {
			this->res.dups++;
			if (kid->g < dup->g) {
				this->res.reopnd++;
				dup->f = dup->f - dup->g + kid->g;
				dup->update(kid->g, parent, op, e.revop);
				computeutil(dup);
				open.pushupdate(dup, dup->ind);
			}
			Kidinfo kinfo(kid->g, dup->h, dup->d);
			nodes->destruct(kid);
			return kinfo;
		}

		kid->d = d.d(e.state);
		if (parent->d - Cost(1) > kid->d)
			kid->d = parent->d - Cost(1);

		kid->h = d.h(e.state);
		// Force h to be consistent along a path so that
		// f is always increasing and the corrections never
		// go negative.
		if (parent->h - e.cost > kid->h)
			kid->h = parent->h - e.cost;

		kid->f = kid->g + kid->h;
		kid->update(kid->g, parent, op, e.revop);
		computeutil(kid);
		closed.add(kid, hash);
		open.push(kid);
		return Kidinfo(kid->g, kid->h, kid->d);
	}

	Node *init(D &d, State &s0) {
		Node *n0 = nodes->construct();
		d.pack(n0->packed, s0);
		n0->g = Cost(0);
		n0->h = n0->f = d.h(s0);
		n0->d = d.d(s0);
		computeutil(n0);
		n0->op = n0->pop = D::Nop;
		n0->parent = NULL;
		return n0;
	}

	// compututil computes the utility value of the given node
	// using corrected estimates of d and h.
	void computeutil(Node *n) {
		double dhat = n->d / (1 - derror);
		double hhat = n->h + dhat * herror;
		double fhat = hhat + n->g;
		n->t = timeper * dhat;
		n->u = -(wf * fhat + wt * n->t);
	}

	// updatetime runs a simple state machine (from Wheeler's BUGSY
	// implementation) that estimates the node expansion rate.
	void updatetime(void) {
		double now;

		switch (state) {
		case WaitTick:	// wait until the clock ticks
			now = walltime();
			if (now <= lasttick)
				break;
			starttime = now;
			state = ExpandSome;
			break;

		case ExpandSome:	// expand a batch of nodes
			nexp++;
			if (nexp < batchsize)
				break;
			lasttick = walltime();
			state = WaitExpand;
			break;

		case WaitExpand:	// estimate the exps for the next tick and reset
			nexp++;
			now = walltime();
			if (now <= lasttick)	// clock hasn't ticked yet
				break;
			updateopen();
			timeper = (now - starttime) / nexp;
			// Re-check the time after 1.8*nexp expansions.
			// 1.8 is from Wheeler's bugsy_old.ml code.  This
			// should be geometrically increasing in nexp to
			// ensure that we only sort the open list a logarithmic
			// number of times.
			batchsize = nexp * 9 / 5;
			nexp = 0;
			starttime = now;
			state = ExpandSome;
			break;

		default:
			fatal("Unknown update time state: %d\n", state);
		}
	}

	void updateopen(void) {
		nresort++;
		for (int i = 0; i < open.size(); i++)
			computeutil(open.at(i));
		open.reinit();
	}

	double wf, wt;

	// heuristic correction
	unsigned long navgh, navgd;
	double herror, derror;

	// for nodes-per-second estimation
	double timeper;
	unsigned long nresort, batchsize, nexp;
	double starttime, lasttick;
	int state;

	BinHeap<Node, Node*> open;
 	ClosedList<SearchNode<D>, SearchNode<D>, D> closed;
	Pool<Node> *nodes;
};
