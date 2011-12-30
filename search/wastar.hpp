#include "../search/search.hpp"
#include <boost/pool/object_pool.hpp>

void fatal(const char*, ...);	// utils.hpp

template <class D> struct Wastar : public SearchAlgorithm<D> {

	typedef typename D::State State;
	typedef typename D::PackedState PackedState;
	typedef typename D::Cost Cost;
	typedef typename D::Oper Oper;

	struct Node : SearchNode<D> {
		double f, fprime;

		static bool pred(Node *a, Node *b) {
			if (a->fprime == b->fprime) {
				if (a->f == b->f)
					return a->g > b->g;
				return a->f < b->f;
			}
			return a->fprime < b->fprime;
		}
	};

	Wastar(int argc, const char *argv[]) :
			SearchAlgorithm<D>(argc, argv), wt(-1.0), closed(30000001) {
		for (int i = 0; i < argc; i++) {
			if (i < argc - 1 && strcmp(argv[i], "-wt") == 0)
				wt = strtod(argv[++i], NULL);
		}

		if (wt < 1)
			fatal("Must specify a weight ≥1 weight using -wt");

		nodes = new boost::object_pool<Node>();
	}

	~Wastar(void) {
		delete nodes;
	}

	Result<D> &search(D &d, typename D::State &s0) {
		SearchAlgorithm<D>::res.start();
		closed.init(d);

		Node *n0 = init(d, s0);
		closed.add(n0);
		open.push(n0);

		while (!open.empty() && !SearchAlgorithm<D>::limit()) {
			Node *n = *open.pop();
			State buf, &state = d.unpack(buf, n->packed);

			if (d.isgoal(state)) {
				SearchAlgorithm<D>::res.goal(d, n);
				break;
			}

			expand(d, n, state);
		}
		SearchAlgorithm<D>::res.finish();

		return SearchAlgorithm<D>::res;
	}

	virtual void reset(void) {
		SearchAlgorithm<D>::reset();
		open.clear();
		closed.clear();
		delete nodes;
		nodes = new boost::object_pool<Node>();
	}

	virtual void output(FILE *out) {
		SearchAlgorithm<D>::output(out);
		closed.prstats(stdout, "closed ");
		dfpair(stdout, "open list type", "%s", "binary heap");
		dfpair(stdout, "node size", "%u", sizeof(Node));
		dfpair(stdout, "weight", "%g", wt);
	}

private:

	void expand(D &d, Node *n, State &state) {
		SearchAlgorithm<D>::res.expd++;

		for (unsigned int i = 0; i < d.nops(state); i++) {
			Oper op = d.nthop(state, i);
			if (op == n->pop)
				continue;
			SearchAlgorithm<D>::res.gend++;
			considerkid(d, n, state, op);
		}
	}

	void considerkid(D &d, Node *parent, State &state, Oper op) {
		Node *kid = nodes->construct();
		typename D::Transition tr(d, state, op);
		kid->g = parent->g + tr.cost;
		d.pack(kid->packed, tr.state);

		unsigned long hash = kid->packed.hash();
		Node *dup = static_cast<Node*>(closed.find(kid->packed, hash));
		if (dup) {
			SearchAlgorithm<D>::res.dups++;
			if (kid->g >= dup->g) {
				nodes->destroy(kid);
				return;
			}
			SearchAlgorithm<D>::res.reopnd++;
			dup->fprime = dup->fprime - dup->g + kid->g;
			dup->f = dup->f - dup->g + kid->g;
			dup->update(kid->g, parent, op, tr.revop);
			open.pushupdate(dup, dup->openind);
			nodes->destroy(kid);
		} else {
			double h = d.h(tr.state);
			kid->f = kid->g + h;
			kid->fprime = kid->g + wt * h;
			kid->update(kid->g, parent, op, tr.revop);
			closed.add(kid, hash);
			open.push(kid);
		}
	}

	Node *init(D &d, State &s0) {
		Node *n0 = nodes->construct();
		d.pack(n0->packed, s0);
		n0->g = 0;
		double h = d.h(s0);
		n0->f = h;
		n0->fprime = wt * h;
		n0->op = n0->pop = D::Nop;
		n0->parent = NULL;
		return n0;
	}

	double wt;
	BinHeap<Node, Node*> open;
 	ClosedList<SearchNode<D>, SearchNode<D>, D> closed;
	boost::object_pool<Node> *nodes;
};
