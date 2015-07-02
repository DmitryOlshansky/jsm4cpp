/**

*/
#pragma once

#include "algorithm.hpp"

class GenericInClose2 : virtual public HybridAlgorithm {
	using HybridAlgorithm::HybridAlgorithm;

	void impl(ExtSet& A, IntSet& B, size_t y){
		if (y == attributes()){
			output(A, B);
			return;
		}
		queue<Rec> q;
		ExtSet C;
		IntSet D;
		for (size_t j = y; j < attributes(); j++) {
			if (!B.has(j)){
				toEmpty(C);
				if (filterExtent(A, j, C)){ // if A == C
					B.add(j);
				}
				else{
					toFull(D);
					partialClosure(C, j, D);
					if (B.equal(D, j)){ // equal up to <j
						q.emplace(move(C), move(D), j);
					}
					else
						stats.fail_canon++;
				}
			}
		}
		output(A, B);
		while (!q.empty()){
			Rec r = move(q.front());
			r.intent.copy(B);
			r.intent.add(r.j);
			processQueueItem(SimpleState{move(r.extent), move(r.intent), r.j+1});
			q.pop();
		}
	}

public:
	using State = SimpleState;
protected:
	virtual void processQueueItem(State&&)=0;
	void algorithm(){
		ExtSet X = ExtSet::newFull();
		IntSet Y = IntSet::newEmpty();
		impl(X, Y, 0);
	}
public:
	void run(SimpleState& state){
		impl(state.extent, state.intent, state.j);
	}
};

using InClose2 = RecursiveAlgorithm<GenericInClose2>;

using ParInClose2 = ForkJoinAlgorithm<GenericInClose2, InClose2>;