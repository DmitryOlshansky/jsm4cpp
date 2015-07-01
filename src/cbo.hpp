/**

*/
#pragma once

#include "algorithm.hpp"

class CbO: virtual public Algorithm {
	using Algorithm::Algorithm;
	// an interation of Close by One algorithm
	void impl(ExtSet& A, IntSet& B, size_t y) {
		output(A, B);
		ExtSet C = ExtSet::newEmpty();
		IntSet D = IntSet::newFull();
		for (size_t j = y; j < attributes(); j++) {
			//cerr << "Layer " << y << " adding " << j << "\n";
			if (!B.has(j)){
				// C empty, D full is a precondition
				if(closeConcept(A, j, C, D)){ // passed min support test
					if (B.equal(D, j)){ // equal up to <j
						impl(C,  D, j + 1);
					}
					else{
						stats.fail_canon++;
					}
				}
				C.clearAll();
				D.setAll();
			}
		}
	}
protected:
	void algorithm(){
		ExtSet X = ExtSet::newFull();
		IntSet Y = IntSet::newFull();
		X.each([&](size_t i){
			Y.intersect(row(i));
		});
		impl(X, Y, 0);
	}
public:
	void run(SimpleState& state){
		impl(state.extent, state.intent, state.j);
	}
};

// Breadth-first CbO modifcation, uses queue to walk CbO call tree in breadth-first manner
// Generalized to overridable strategy w.r.t. handling queue items
class GenericBCbO: virtual public HybridAlgorithm {
	using HybridAlgorithm::HybridAlgorithm;
	// an interation of Close by One algorithm
	void impl(ExtSet& A, IntSet& B, size_t y) {
		output(A, B);
		if(y == attributes())
			return;
		queue<Rec> q;
		ExtSet C;
		IntSet D;
		for (size_t j = y; j < attributes(); j++) {
			if (!B.has(j)){
				if(C.null()){
					C = ExtSet::newEmpty();
					D = IntSet::newFull();
				}
				// C empty, D full is a precondition
				if(closeConcept(A, j, C, D)){ // passed min support test
					if (B.equal(D, j)){ // equal up to <j
						q.emplace(move(C), move(D), j);
						// now C&D are null
					}
					else{
						// reuse existing sets
						stats.fail_canon++;
						C.clearAll();
						D.setAll();
					}
				}
				
			}
		}
		while (!q.empty()){
			Rec r = move(q.front());
			processQueueItem(SimpleState{move(r.extent), move(r.intent), r.j + 1});
			q.pop();
		}
	}
protected:
	void algorithm(){
		ExtSet X = ExtSet::newFull();
		IntSet Y = IntSet::newFull();
		X.each([&](size_t i){
			Y.intersect(row(i));
		});
		impl(X, Y, 0);
	}
	
	
public:
	using State = SimpleState;
protected:
	virtual void processQueueItem(State&&)=0;
public:
	void run(State& state){
		impl(state.extent, state.intent, state.j);
	}
};

// Plain recursive call strategy
using BCbO = RecursiveAlgorithm<GenericBCbO>;

// Use normal CbO for parallel excution part, no need to queue items 
using ParCbO = ForkJoinAlgorithm<GenericBCbO, CbO>;
