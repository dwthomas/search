// Copyright © 2020 the Search Authors under the MIT license. See AUTHORS for the list of authors.                                                             
#pragma once                                                                    
#include "../search/search.hpp"                                                 
#include "../utils/pool.hpp"
#include "utils.h"
#include <cstddef>
#include <limits>
#include <boost/heap/d_ary_heap.hpp>

                                                                                
template <class D> struct BoundedRectangleBeadSearch : public SearchAlgorithm<D> {

	typedef typename D::State State;
	typedef typename D::PackedState PackedState;                                
	typedef typename D::Cost Cost;                                              
	typedef typename D::Oper Oper;

	struct Node;
	struct FComp;
	struct DComp;



	using FQueue = boost::heap::d_ary_heap<Node *, boost::heap::arity<4>, boost::heap::mutable_<true>, boost::heap::compare<FComp>>;
	typedef typename FQueue::handle_type FHandle_t;
	using DQueue = boost::heap::d_ary_heap<Node *, boost::heap::arity<4>, boost::heap::mutable_<true>, boost::heap::compare<DComp>>;
	typedef typename DQueue::handle_type DHandle_t;

	struct Node {
		//OpenList<DOps, Node, double> * open;
		Node *parent;
		PackedState state;
		Oper op, pop;
		int d, depth;
		Cost f, g;
		FHandle_t cleanup_handle;
		DHandle_t open_handle;
		FHandle_t pruned_handle;
		bool in_cleanup, in_open, in_pruned;
		DQueue * open_ptr;
		FQueue * pruned_ptr;
  
		Node() : cleanup_handle(),open_handle(),pruned_handle(),in_cleanup(false),in_open(false),in_pruned(false){
		}

		static ClosedEntry<Node, D> &closedentry(Node *n) {
			return n->closedent;
		}

		static PackedState &key(Node *n) {
			return n->state;
		}

		inline bool check() const{
			return in_cleanup && (in_open || in_pruned) && !(in_open && in_pruned);
		}

		inline friend std::ostream& operator<< (std::ostream& stream, const Node& n){
            stream << " g:" << n.g 
			<< ", f:" << n.f 
			<< ", d:" << n.d 
			//<< ", depth:" << n.depth 
			<< " opens:" << n.in_cleanup 
			<< n.in_open
			<< n.in_pruned;
            return stream;
        }

		private:
			ClosedEntry<Node, D> closedent;
		
	};
	struct FComp{
		inline bool operator()(const Node* a, const Node* b){
			if(a->f == b->f){
                return a->g < b->g;
            }
            return a->f > b->f;
		}
	};

	struct DComp{
		inline bool operator()(const Node* a, const Node* b){
			if(a->d == b->d){
                return a->g < b->g;
            }
            return a->d > b->d;
		}
	};
	
	// struct DOps {
	// 	/* Set index of node on open list. */
	// 	static void setind(Node *n, int i) {
	// 		n->openind = i;
	// 	}

	// 	/* Get index of node on open list. */
	// 	static int getind(const Node *n) {
	// 		return n->openind;
	// 	}

	// 	/* Indicates whether Node a has better value than Node b. */
	// 	static bool pred(Node *a, Node *b) {
	// 		if (a->d == b->d)
	// 			return a->g > b->g;
	// 		return a->d < b->d;
	// 	}

	// 	/* Priority of node. */
	// 	static Cost prio(Node *n) {
	// 		return n->d;
	// 	}

	// 	/* Priority for tie breaking. */
	// 	static Cost tieprio(Node *n) {
	// 		return n->g;
	// 	}
	// };

	// struct FOps {
	// 	static void setind(Node *n, int i) {
	// 		n->cleanupind = i;
	// 	}

	// 	static int getind(const Node *n) {
	// 		return n->cleanupind;
	// 	}
	
	// 	static bool pred(Node *a, Node *b) {
	// 		if (a->f == b->f) {
	// 			//if (a->d == b->d)
	// 			return a->g > b->g;
	// 			//return a->d < b->d;
	// 		}
	// 		return a->f < b->f;
	// 	}	

	// 	/* Priority of node. */
	// 	static Cost prio(Node *n) {
	// 		return n->f;
	// 	}

	// 	/* Priority for tie breaking. */
	// 	static Cost tieprio(Node *n) {
	// 		return n->g;
	// 	}
	// };


	// struct PrunedFOps {
	// 	static void setind(Node *n, int i) {
	// 		n->prunedind = i;
	// 	}

	// 	static int getind(const Node *n) {
	// 		return n->prunedind;
	// 	}
	
	// 	static bool pred(Node *a, Node *b) {
	// 		if (a->f == b->f) {
	// 			//if (a->d == b->d)
	// 			return a->g > b->g;
	// 			//return a->d < b->d;
	// 		}
	// 		return a->f < b->f;
	// 	}	

	// 	/* Priority of node. */
	// 	static Cost prio(Node *n) {
	// 		return n->f;
	// 	}

	// 	/* Priority for tie breaking. */
	// 	static Cost tieprio(Node *n) {
	// 		return n->g;
	// 	}
	// };

	struct RingNode {
		RingNode *next;
		RingNode *prev;
		DQueue * list;
		FQueue * pruned; 

		RingNode(DQueue *l) {
			next = NULL;
			prev = NULL;
			list = l;
			pruned = new FQueue();
		}
		~RingNode() {
			delete list;
			delete pruned;
		}
	};
  
	struct Ring {
		RingNode *begin;
		RingNode *end;
		int maxsize;
		int size;
		int removed;
		int reused;

		Ring() {
			begin = new RingNode(NULL);
			end = new RingNode(NULL);
			begin->next = end;
			begin->prev = end;
			end->next = begin;
			end->prev = begin;
			maxsize = 0;
			size = 0;
			removed = 0;
			reused = 0;
		}

		~Ring() {
			while(begin->next != end) {
			RingNode *temp = begin;
			begin = begin->next;
			delete temp;
			}
			delete begin;
			delete end;
		}

		void move_after(RingNode *n, RingNode *b) {
			n->prev->next = n->next;
			n->next->prev = n->prev;
			b->next->prev = n;
			n->next = b->next;
			n->prev = b;
			b->next = n;
		}

		void add() {
			if(begin->prev == end) {
			RingNode *n = new RingNode(new DQueue());
			n->prev = end->prev;
			n->next = end;
			end->prev->next = n;
			end->prev = n;
			maxsize++;
			size++;
			} else {
			move_after(begin->prev, end->prev);
			size++;
			reused++;
			}
		}

		void remove_rest(RingNode *from) {
			RingNode *a = from->next;
			RingNode *b = end->prev;

			end->prev = from;
			from->next = end;
			a->prev = end;
			b->next = end->next;
			end->next->prev = b;
			end->next = a;
		}

		void remove() {
			move_after(begin->next, end);
			size--;
			removed++;
		} 
	};
  

	BoundedRectangleBeadSearch(int argc, const char *argv[]):SearchAlgorithm<D>(argc, argv),closed(30000001){
		bounded_factor = std::numeric_limits<double>::infinity();
		dropdups = false;
		dump = false;
		exponential_h = false;
		delta_height = 1;
		delta_base = 1;
		for (int i = 0; i < argc; i++) {
			if (strcmp(argv[i], "-dropdups") == 0)
				dropdups = true;
			if (i < argc - 1 && (strcmp(argv[i], "-dH") == 0 ||
								strcmp(argv[i], "-aspect") == 0))
				delta_height = strtod(argv[++i], NULL);
			if (i < argc - 1 && strcmp(argv[i], "-dB") == 0)
				delta_base = strtod(argv[++i], NULL);
			if (i < argc - 1 && strcmp(argv[i], "-wt") == 0)
				bounded_factor = strtod(argv[++i], NULL);
			if (strcmp(argv[i], "-dump") == 0)
				dump = true;
			if (strcmp(argv[i], "-expo") == 0)
				exponential_h = true;

		}
		nodes = new Pool<Node>();
	}

	~BoundedRectangleBeadSearch() {
		delete nodes;
	}

	Node *dedup(D &d, Node *n) {

	  if(cand && n->f >= cand->g) {
		nodes->destruct(n);
		return NULL;
	  }
	  
	  unsigned long hash = n->state.hash(&d);
	  Node *dup = closed.find(n->state, hash);
	  if(!dup) {
		closed.add(n, hash);
	  } else {
		SearchAlgorithm<D>::res.dups++;
		if(!dropdups && n->g < dup->g) {
		  SearchAlgorithm<D>::res.reopnd++;
					
		  dup->f = dup->f - dup->g + n->g;
		  dup->g = n->g;
		  dup->d = n->d;
		  dup->parent = n->parent;
		  dup->op = n->op;
		  dup->pop = n->pop;
		} else {
		  nodes->destruct(n);
		  return NULL;
		}
	  }

	  return n;
	}

	void search(D &d, typename D::State &s0) {
		this->start();
		closed.init(d);

		Node *n0 = init(d, s0);
		closed.add(n0);

		openlists.add();
		auto open_it = openlists.end->prev;
		auto last_filled = open_it;
		
		if(dump) {
		  fprintf(stderr, "depth,expnum,state,g\n");
		  fprintf(stderr, "0,%lu,", SearchAlgorithm<D>::res.expd);
		  State buf, &state = d.unpack(buf, n0->state);
		  d.dumpstate(stderr, state);
		  fprintf(stderr, ",%f\n", (float)n0->g);
		}

		open_count = 0;
		expand(d, n0, s0, 0, *open_it->list);
		
		int width_inc = int(delta_base);
		int depth_todo = int(delta_height);
		int n_iter = 0;
		int exp_todo = width_inc;

		sol_count = 0;
		depth = 1;

		dfrowhdr(stdout, "incumbent", 5, "num", "nodes expanded", "nodes generated", "solution cost", "wall time");
		bool done = false;
    
		while (!done && !SearchAlgorithm<D>::limit()) {
		  done = true;
		  depth++;
		  n_iter++;

		  open_it = openlists.begin->next;
		  auto& open = open_it->list;
		  auto& pruned = open_it->pruned;

		  openlists.add();
		  depth_todo = int(delta_height);
		  
		  if (exponential_h){
			delta_height = delta_height*2;
		  }
		  
		  int curr_depth = openlists.removed;
		  
		  // loop through all open lists, adding more at the end if needed
		  while(open_it->next != openlists.end) {
			curr_depth++;
			
			if(open_it->next->next != openlists.end) {
			  exp_todo = width_inc;
			} else {
			  exp_todo = n_iter * width_inc;
			  
			  // create new open lists for this iteration
			  if(depth_todo > 0 && !open->empty()) {
				openlists.add();
				depth_todo--;
			  } else {
				break;
			  }
			}
			exp_todo = std::min((int)open->size(), exp_todo);
			
			Node *arr[exp_todo];
			bool some_exp = false;
			// replace from pruned
			while(!some_exp && (!pruned->empty() || !open->empty())){
				while(!pruned->empty() && (pruned->top()->f <= bounded_factor * min_f())){
					assert(pruned->top()->in_pruned && pruned->top()->in_cleanup);
					auto handle = open->push(pruned->top());
					(*handle)->open_handle = handle;
					(*handle)->in_open = true;
					(*handle)->in_pruned = false;
					(*handle)->open_ptr = open;
					(*handle)->pruned_ptr = nullptr;
					pruned->pop();
				}

				for(int i = 0; i < exp_todo; i++) {
					Node *n = NULL;
						
					while(!n && !open->empty()) {
						n = open->top();
						// std::cerr << *n << "\n"; 
						// assert(n->cleanupind > -1);
						assert(n->in_cleanup);
						n->in_open = false;	
						n->open_handle = DHandle_t();
						n->open_ptr = nullptr;
						open->pop();
						if(n != nullptr && n->f > bounded_factor * min_f()){
							n->pruned_handle = pruned->push(n);
							n->in_pruned = true;
							n->pruned_ptr = pruned;
							n = nullptr;
							continue;
						}
						else{
							n->in_cleanup = false;
							cleanup.erase(n->cleanup_handle);
						}
						n = dedup(d, n);
						open_count--;
					}
					arr[i] = n;
					if(n){
						some_exp = true;
					}
					else{
						while(!pruned->empty() && pruned->top()->f > bounded_factor * min_f()){
							n = nullptr;
							while(!n && !cleanup.empty()){
								n = cleanup.top();
								assert(n->check());
								n->in_cleanup = false;
								cleanup.pop();
								assert(n); 
								//std::cerr << n->cleanupind << "/" << cleanup.size() << " " << n->openind << "/" << open->size() << " " << n->prunedind<< "/" << pruned->size() << "\n";
								if(n->in_open){
									//std::cerr << *n << "\n";
									n->open_ptr->erase(n->open_handle);
									n->in_open = false;
								}
								if(n->in_pruned){
									n->pruned_ptr->erase(n->pruned_handle);
									n->in_pruned = false;
								}
								n = dedup(d, n);
							}
							State buf, &state = d.unpack(buf, n->state);
							expand(d, n, state, curr_depth, *open);
						}
					}
				}
				
				
				
			}

			// move to next open list
		    open_it = open_it->next;
			open = open_it->list;

			// record the last filled open list for pruning
			if(some_exp)
			  last_filled = open_it;

			// prune if this is the shallowest depth open list and it is empty
			if(!some_exp && done) {
				openlists.remove();
				continue;
			}

			// expand one or more nodes, based on slope
			for(int i = 0; i < exp_todo; i++) {
			  Node *n = arr[i];
			  if(!n)
				continue;
			  
			  State buf, &state = d.unpack(buf, n->state);
			  if(dump) {
				fprintf(stderr, "%d,%lu,", curr_depth,
						SearchAlgorithm<D>::res.expd);
				d.dumpstate(stderr, state);
				fprintf(stderr, ",%f\n", (float)n->g);
			  }
			  expand(d, n, state, curr_depth, *open);
			}
			
			done = false;
		  }

		  // prune sequences of empty open lists from end
		  if(last_filled != openlists.end->prev) {
			openlists.remove_rest(last_filled);
		  }
		}

		if(cand) {
		  solpath<D, Node>(d, cand, this->res);
		  done = true;
		}
		this->finish();
	}

	virtual void reset() {
		SearchAlgorithm<D>::reset();
		//open->clear();
		closed.clear();
		delete nodes;
		nodes = new Pool<Node>();
	}

	virtual void output(FILE *out) {
		SearchAlgorithm<D>::output(out);
		closed.prstats(stdout, "closed ");
		dfpair(stdout, "open lists created", "%d", openlists.maxsize);
		//dfpair(stdout, "open list type", "%s", open->kind());
		dfpair(stdout, "node size", "%u", sizeof(Node));
	}


	private:
		double min_f(){
			assert(!cleanup.empty());
			if(cleanup.empty()){
				return std::numeric_limits<double>::infinity();
			}
			//std::cerr << cleanup.size() << " " << cleanup.peek() << "\n";
			return cleanup.top()->f;
		}
		void expand(D &d, Node *n, State &state, int curr_depth, DQueue& open) {
			SearchAlgorithm<D>::res.expd++;

			typename D::Operators ops(d, state);
			for (unsigned int i = 0; i < ops.size(); i++) {
				if (ops[i] == n->pop)
					continue;
				SearchAlgorithm<D>::res.gend++;
				considerkid(d, n, state, ops[i], curr_depth, open);
			}
		}

		void considerkid(D &d, Node *parent, State &state, Oper op, int curr_depth, DQueue& open) {
			Node *kid = nodes->construct();
			assert (kid);
			// std::cerr << parent->f << " ";
			// std::cerr << "bound: " << bounded_factor;
			// std::cerr << " * " << min_f() << "\n"; 
			typename D::Edge e(d, state, op);
			kid->g = parent->g + e.cost;
			d.pack(kid->state, e.state);

			kid->f = kid->g + d.h(e.state);
			kid->d = d.d(e.state);
			kid->parent = parent;
			kid->op = op;
			kid->pop = e.revop;
			
			State buf, &kstate = d.unpack(buf, kid->state);
			if (d.isgoal(kstate) && (!cand || kid->g < cand->g)) {
			
				if(dump) {
					fprintf(stderr, "%d,%lu,", curr_depth,
							SearchAlgorithm<D>::res.expd);
					d.dumpstate(stderr, kstate);
					fprintf(stderr, ",%f\n", (float)kid->g);
				}
				
				cand = kid;
				sol_count++;
				dfrow(stdout, "incumbent", "uuugg", sol_count, this->res.expd,
						this->res.gend, (float)cand->g,
						walltime() - this->res.wallstart);
				return;
			} 
			else if(cand && cand->g <= kid->f) {
				nodes->destruct(kid);
				return;
			}

			open_count++;
			kid->open_handle = open.push(kid);
			kid->cleanup_handle = cleanup.push(kid);
			kid->in_cleanup = true;
			kid->in_open = true;
			kid->open_ptr = &open;
			kid->in_pruned = false;
			kid->pruned_ptr = nullptr;
			//std::cerr << "n: " << cleanup.size() << " min: " << min_f() << "\n"; 
		}

		Node *init(D &d, State &s0) {
			Node *n0 = nodes->construct();
			d.pack(n0->state, s0);
			n0->d = d.d(s0);
			n0->g = Cost(0);
			n0->f = d.h(s0);
			n0->pop = n0->op = D::Nop;
			n0->parent = NULL;
			cand = NULL;
			return n0;
		}

		bool dropdups;
		bool dump;
		bool exponential_h;
		Ring openlists;
		FQueue cleanup;
		ClosedList<Node, Node, D> closed;
		Pool<Node> *nodes;
		Node *cand;
		int width;
		int depth;
		int open_count;
		int sol_count;
		int delta_height;
		int delta_base;
		double bounded_factor;
  
};
