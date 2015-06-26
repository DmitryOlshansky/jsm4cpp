/**

*/
#pragma once

#include "algorithm.hpp"

class CbO: virtual public Algorithm {
	using Algorithm::Algorithm;
	// an interation of Close by One algorithm
	void impl(ExtSet A, IntSet B, size_t y) {
		output(A, B);
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet C = exts.newEmpty();
		IntSet D = ints.newFull();
		for (size_t j = y; j < attributes(); j++) {
			if (!B.has(j)){
				// C empty, D full is a precondition
				if(closeConcept(A, j, C, D)){ // passed min support test
					if (B.equal(D, j)){ // equal up to <j
						impl(C, D, j + 1);
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

	void algorithm(){
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet X = exts.newFull();
		IntSet Y = ints.newFull();
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


class ParCbO: virtual public HybridAlgorithm, public ParallelAlgorithm<SimpleState, CbO> {
	using Algorithm::Algorithm;
	// an interation of Close by One algorithm
	void parCboImpl(ExtSet A, IntSet B, size_t y, size_t rec_level) {
		output(A, B);
		if(y == attributes())
			return;
		queue<Rec> q;
		IntSet::Pool ints(attributes() - y);
		ExtSet::Pool exts(attributes() - y);
		ExtSet C;
		IntSet D;
		for (size_t j = y; j < attributes(); j++) {
			if (!B.has(j)){
				if(C.empty()){
					C = exts.newEmpty();
					D = ints.newFull();
				}
				// C empty, D full is a precondition
				if(closeConcept(A, j, C, D)){ // passed min support test
					if (B.equal(D, j)){ // equal up to <j
						q.emplace(C, D, j);
						C = ExtSet();
						D = IntSet();
					}
					else{
						// reuse existing set
						stats.fail_canon++;
						C.clearAll();
						D.setAll();
					}
				}
				
			}
		}
		while (!q.empty()){
			Rec r = q.front();
			if (rec_level == parLevel())
				schedule(SimpleState{r.extent, r.intent, r.j + 1});
			else
				parCboImpl(r.extent, r.intent, r.j + 1, rec_level+1);
			q.pop();
		}
	}

	void serialStep(){
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet X = exts.newFull();
		IntSet Y = ints.newFull();
		X.each([&](size_t i){
			Y.intersect(row(i));
		});
		parCboImpl(X, Y, 0, 0);
	}
};
