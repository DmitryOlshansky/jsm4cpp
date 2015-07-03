/**

*/
#pragma once

#include "algorithm.hpp"


class GenericInClose3: virtual public HybridAlgorithm {
	using HybridAlgorithm::HybridAlgorithm;

	void impl(ExtSet& A, IntSet& B, size_t y, CompIntSet* N){
		if (y == attributes()){
			output(A, B);
			return;
		}
		queue<Rec> q;
		ExtSet C;
		IntSet D;
		CompIntSet* M = N + attributes();
		for (size_t j = y; j < attributes(); j++) {
			M[j] = N[j];
			if (!B.has(j)){
				if (N[j].null() || N[j]->subsetOf(B, j)){ // subset of (considering attributes < j)
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
						else {
							M[j] = move(D);
							stats.fail_canon++;
						}
					}
				}
				else
					stats.fail_fast++;
			}
		}
		output(A, B);
		while (!q.empty()){
			Rec r = move(q.front());
			r.intent.copy(B);
			r.intent.add(r.j);
			processQueueItem(ExtendedState{move(r.extent), move(r.intent), r.j + 1, M, attributes()});
			q.pop();
		}
	}

public:
	using State = ExtendedState;
	virtual void processQueueItem(State&&)=0;
protected:
	void algorithm(){
		// intents with implied error, see FCbO papper
		CompIntSet* implied = new CompIntSet[(attributes()+1)*attributes()]; //overkill for parallel versions..
		ExtSet X = ExtSet::newFull();
		IntSet Y = IntSet::newEmpty();
		impl(X, Y, 0, implied);
		// can't delete here because of parellel version that follows up using these sets
		// delete[] implied;
	}
public:
	void run(ExtendedState& state){
		impl(state.extent, state.intent, state.j, state.implied);
	}
};

using InClose3 = RecursiveCalls<GenericInClose3>;

using ParInClose3 = ForkJoin<GenericInClose3, InClose3>;

using FParInClose3 = FairForkJoin<GenericInClose3, InClose3>;

using TPInClose3 = WithThreadPool<GenericInClose3, InClose3>;
