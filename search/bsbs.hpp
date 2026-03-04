// Copyright © 2013 the Search Authors under the MIT license. See AUTHORS for the list of authors.
#include "../search/search.hpp"
#include "../utils/pool.hpp"
#include "closedlist.hpp"
#include <cstddef>

void fatal(const char*, ...);	// utils.hpp

template <class D> struct BSBS : public SearchAlgorithm<D> {

	typedef typename D::State State;
	typedef typename D::PackedState PackedState;
	typedef typename D::Cost Cost;
	typedef typename D::Oper Oper;

	std::size_t n_beam_expansions = 0;
	std::size_t n_astar_expansions = 0;

	// struct SearchState{
	// 	private:
	// 		enum RoundRobinState {f, fhat, d};
	// 		RoundRobinState state;
	// 	public:
	// 		SearchState():state(RoundRobinState::f){}
	// 		SearchState(const SearchState& s):state(s.state){}

	// 		bool is_d() const {
	// 			return state == RoundRobinState::d;
	// 		}

	// 		bool is_f() const{
	// 			return state == RoundRobinState::f;
	// 		}

	// 		bool is_fhat() const {
	// 			return state == RoundRobinState::fhat;
	// 		}

	// 		SearchState operator++(int){
	// 			auto old_state = SearchState(*this);
	// 			switch(state){
	// 				case RoundRobinState::f:
	// 					state = RoundRobinState::fhat;
	// 					break;
	// 				case RoundRobinState::fhat:
	// 					state = RoundRobinState::d;
	// 					break;
	// 				case RoundRobinState::d:
	// 					state = RoundRobinState::f;
	// 					break;
	// 			} 
	// 			return old_state;
	// 		}
	// };

	struct Node {

		ClosedEntry<Node, D> closedent;

		// values for tracking location in focal, open, and f-ordered list
		bool open;
		int focalind;
		int cleanupind;

		Node *parent;
		PackedState state;
		Oper op, pop;
		Cost f, g, h;
		int d;
		double dhat;

		Node() : open(false), focalind(-1), cleanupind(-1) {
		}

		static ClosedEntry<Node, D> &closedentry(Node *n) {
			return n->closedent;
		}

		static PackedState &key(Node *n) {
			return n->state;
		}
	};

	struct FOps {
		static void setind(Node *n, int i) {
			n->cleanupind = i;
		}

		static int getind(const Node *n) {
			return n->cleanupind;
		}
	
		static bool pred(Node *a, Node *b) {
			if (a->f == b->f) {
				if (a->d == b->d)
					return a->g > b->g;
				return a->d < b->d;
			}
			return a->f < b->f;
		}	
	};
  
	struct DHatOps {
		static void setind(Node *n, int i) {
			n->focalind = i;
		}

		static int getind(const Node *n) {
			return n->focalind;
		}
	
		static bool pred(Node *a, Node *b) {
			if (a->dhat == b->dhat) {
				if (a->f == b->f)
					return a->g > b->g;
				return a->f < b->f;
			}
			return a->dhat < b->dhat;
		}	
	};

	// struct FHatOps {

	// 	static double getvalue(const Node *n) {
	// 		return n->fhat;
	// 	}
	
	// 	static bool pred(Node *a, Node *b) {
	// 		/*
	// 		if (a->fhat == b->fhat) {
	// 			if (a->d == b->d)
	// 				return a->g > b->g;
	// 			return a->d < b->d;
	// 		}
	// 		*/
	// 		return a->fhat < b->fhat;
	// 	}	
	// };

	BSBS(int argc, const char *argv[]) :
			SearchAlgorithm<D>(argc, argv), herror(0), derror(0), 
			dropdups(false), wt(-1.0), closed(30000001) {
		for (int i = 0; i < argc; i++) {
			if (i < argc - 1 && strcmp(argv[i], "-wt") == 0)
				wt = strtod(argv[++i], NULL);
			if (i < argc - 1 && strcmp(argv[i], "-width") == 0)
				width = atoi(argv[++i]);
			if (strcmp(argv[i], "-dropdups") == 0)
				dropdups = true;
		}

		if (wt < 1)
			fatal("Must specify a weight ≥1 weight using -wt");
		if (width < 1)
			fatal("Must specify a >0 beam width using -width");
		nodes = new Pool<Node>();
	}

	~BSBS() {
		delete nodes;
	}

	Node *select_node() {
	  	Node *bestDHat = NULL;
		if(!focal.empty()) {
			bestDHat = *focal.front();
		}
		//   Node *bestFHat = open.front();
		// std::cerr << "cleanup front\n";
		if (cleanup.empty()){
			// std::cerr << "cleanup empty:\n";
		}
		else {
			// std::cerr << "cleanup not empty:\n";
		}
		Node *bestF = *cleanup.front();
		// std::cerr << "post-cleaup front\n";
		//   auto ss = SearchState(search_state++);

		// if(ss.is_d() && bestDHat && bestDHat->fhat <= wt*bestF->f) {
		if(focal.empty()){
			// std::cerr << "focal empty:\n";
			cleanup.remove(bestF);
			// std::cerr << "returning bestF\n";
			if(bestF->focalind >= 0) {
				focal.remove(bestF);
			}
			return bestF;
		}
		focal.remove(bestDHat);
		cleanup.remove(bestDHat);
		return bestDHat;
	}

	void search(D &d, typename D::State &s0) {
		this->start();
		closed.init(d);

		Node *n0 = init(d, s0);
		// closed.add(n0);
		focal.push(n0);
		cleanup.push(n0);

		// fmin = n0->f;

		bool isIncrease;
		// Dummy node to represent weighted n0.
		Node *dummy = new Node();
		dummy->d = n0->d;
		dummy->g = n0->g;
		dummy->h = n0->h;
		dummy->f = n0->g + n0->h;
		dummy->dhat = wt * dummy->d;

		// open.updateCursor(dummy, isIncrease);

		while (!cleanup.empty() && !SearchAlgorithm<D>::limit()) {
			Node **beam = new Node*[width];
			int c = 0;
			while(c < width && !focal.empty()) {
				// std::cerr << "making beam\n";
				// process focal into beam
				Node *n = select_node(); // focal is not empty, so will be from focal
				// unsigned long hash = n->state.hash(&d);
				// Node *dup = closed.find(n->state, hash);
				// if(!dup) {
				// //   closed.add(n, hash);
				// } else {
				// 	std::cerr << "duplicate!" << std::endl;
				//   SearchAlgorithm<D>::res.dups++;
				//   if(!dropdups && n->g < dup->g) {
				// 	SearchAlgorithm<D>::res.reopnd++;
					
				// 	dup->f = dup->f - dup->g + n->g;
				// 	dup->g = n->g;
				// 	dup->d = n->d;
				// 	dup->parent = n->parent;
				// 	dup->op = n->op;
				// 	dup->pop = n->pop;
				//   } else {
				// 	continue;
				//   }
				// }

				beam[c] = n;
				c++;
			}
			// std::cerr << "beam size: " << c << std::endl;
			
			if (c == 0){
				n_astar_expansions++;
				// Do A* expansion
				// std::cerr << "A expansion: " << std::endl;
				Node *n = select_node();
				// std::cerr << "node selected" << std::endl;
				State buf, &state = d.unpack(buf, n->state);

				if (d.isgoal(state)) {
					solpath<D, Node>(d, n, this->res);
					break;
				}

				expand(d, n, state);
			}
			else{
				// std::cerr << "Beam expansion: " << std::endl;
				bool done = false;
				// Do beam expansions, make sure the nodes get deleted when removed from A* heap
				for(int i = 0; i < c && !done && !SearchAlgorithm<D>::limit(); i++) {
					Node *n = beam[i];
					State buf, &state = d.unpack(buf, n->state);
					if (d.isgoal(state)) {
						solpath<D, Node>(d, n, this->res);
						done = true;
						break;
					}
					expand(d, n, state);
					n_beam_expansions++;
				}
				if (done){
					break;
				}
			}
			delete[] beam;
		}
		this->finish();
	}

	virtual void reset() {
		SearchAlgorithm<D>::reset();
		focal.clear();
		cleanup.clear();
		closed.clear();
		delete nodes;
		nodes = new Pool<Node>();
		herror = 0;
		derror = 0;
	}

	virtual void output(FILE *out) {
		SearchAlgorithm<D>::output(out);
		closed.prstats(stdout, "closed ");
		dfpair(stdout, "open list type", "%s", "binary heap");
		dfpair(stdout, "node size", "%u", sizeof(Node));
		dfpair(stdout, "weight", "%lg", wt);
		dfpair(out, "h error last", "%g", herror);
		dfpair(out, "d error last", "%g", derror);
		dfpair(stdout, "width", "%u", width);
		dfpair(stdout, "Astar expansions", "%u", n_astar_expansions);
		dfpair(stdout, "beam expansions", "%u", n_beam_expansions);
	}

private:

	Cost fmin(){
		Node * bestF = *cleanup.front();
		return bestF->f;
	}

	void expand(D &d, Node *n, State &state) {
		SearchAlgorithm<D>::res.expd++;

        double herrnext = 0;
		double derrnext = 0;

        Node *bestkid = NULL;

		typename D::Operators ops(d, state);
		for (unsigned int i = 0; i < ops.size(); i++) {
			if (ops[i] == n->pop)
				continue;
			SearchAlgorithm<D>::res.gend++;

			Node *kid = nodes->construct();
			typename D::Edge e(d, state, ops[i]);
			kid->g = n->g + e.cost;
			d.pack(kid->state, e.state);

			unsigned long hash = kid->state.hash(&d);
			Node *dup = static_cast<Node*>(closed.find(kid->state, hash));
			if (dup) {
				this->res.dups++;
				if (dropdups || kid->g >= dup->g) {
					nodes->destruct(kid);
					continue;
				}
				if (dup->cleanupind >= 0) {
					this->res.reopnd++;
				}
				dup->f = dup->f - dup->g + kid->g;
				dup->g = kid->g;
				double dhat = dup->d / (1 - derror);
				// double hhat = dup->h + (herror * dhat);
				dup->dhat = dhat;
				dup->parent = n;
				dup->op = ops[i];
				dup->pop = e.revop;
				cleanup.pushupdate(dup, dup->cleanupind);
				if(dup->f <= wt * fmin()) {
				 	focal.pushupdate(dup, dup->focalind);
				 } else if(dup->focalind >= 0) {
					focal.remove(dup);
				}
				nodes->destruct(kid);
 
				if (!bestkid || dup->f < bestkid->f)
					bestkid = dup;
			} else {
				typename D::Cost h = d.h(e.state);
				kid->h = h;
				kid->f = kid->g + h;
				kid->d = d.d(e.state);
				double dhat = kid->d / (1 - derror);
				// double hhat = kid->h + (herror * dhat);
				kid->dhat = dhat;
				kid->parent = n;
				kid->op = ops[i];
				kid->pop = e.revop;
				closed.add(kid, hash);
				cleanup.push(kid);
				if(kid->f <= wt * fmin()) {
					focal.push(kid);
				}
 
				if (!bestkid || kid->f < bestkid->f)
					bestkid = kid;
			}
		}

		if (bestkid) {
		  double herr =  bestkid->f - n->f;
		  if (herr < 0)
		  	herr = 0;
		  double pastErrSum = herror * ((SearchAlgorithm<D>::res.expd)+imExp-1);
		  herrnext = (herr + pastErrSum)/((SearchAlgorithm<D>::res.expd)+imExp);
		  // imagine imExp of expansions with no error
		  // regulates error change in beginning of search

		  double derr = (bestkid->d+1) - n->d;
		  if (derr < 0)
			derr = 0;
		  if (derr >= 1)
			derr = 1 - geom2d::Threshold;
		  double pastDSum = derror * ((SearchAlgorithm<D>::res.expd)+imExp-1);
		  derrnext = (derr + pastDSum)/((SearchAlgorithm<D>::res.expd)+imExp);
		  // imagine imExp of expansions with no error
		  // regulates error change in beginning of search
		  
		  herror = herrnext;
		  derror = derrnext;
		}

	}

	Node *init(D &d, State &s0) {
		Node *n0 = nodes->construct();
		d.pack(n0->state, s0);
		n0->h = d.h(s0);
		n0->g = Cost(0);
		n0->f = n0->h + n0->g;
		n0->dhat = n0->d;
		n0->d = d.d(s0);
		n0->op = n0->pop = D::Nop;
		n0->parent = NULL;
		return n0;
	}

	double herror;
	double derror;

	bool dropdups;
	double wt;
	int width;
	BinHeap<DHatOps, Node*> focal;
	BinHeap<FOps, Node*> cleanup;
 	ClosedList<Node, Node, D> closed;
	Pool<Node> *nodes;
	double fhatmin;

    int imExp = 10;

};