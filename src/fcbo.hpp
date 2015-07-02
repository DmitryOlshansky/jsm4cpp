/**

*/
#pragma once

#include "algorithm.hpp"

class GenericFCbO : virtual public HybridAlgorithm {
	using HybridAlgorithm::HybridAlgorithm;

	void impl(ExtSet& A, IntSet& B, size_t y, CompIntSet* N){
		output(A, B);
		if (y == attributes())
			return;
		queue<Rec> q;
		CompIntSet* M = N + attributes();
		ExtSet C;
		IntSet D;
		// note that M[0..y] may point to stale sets which however doesn't matter
		for (size_t j = y; j < attributes(); j++) {
			M[j] = N[j];
			if (!B.has(j)){
				if (N[j].null() || N[j]->subsetOf(B, j)){ // subset of (considering attributes < j)
					// C empty, D full is a precondition
					toEmpty(C);
					toFull(D);
					if(closeConcept(A, j, C, D) && B.equal(D, j)){ // equal up to <j
						q.emplace(move(C), move(D), j); //lose both C&D
					}
					else {
						stats.fail_canon++;
						M[j] = move(D); // lose D
					}
				}
				else
					stats.fail_fast++;
			}
		}
		
		while (!q.empty()){
			Rec r = move(q.front());
			processQueueItem(ExtendedState{move(r.extent), move(r.intent), r.j + 1, M, attributes()});
			q.pop();
		}
	}
public:
	using State = ExtendedState;
protected:
	virtual void processQueueItem(State&&)=0;
protected:
	void algorithm(){
		ExtSet X = ExtSet::newFull();
		IntSet Y = IntSet::newFull();
		X.each([&](size_t i){
			Y.intersect(row(i));
		});
		// intents with implied error, see FCbO papper
		CompIntSet* implied = new CompIntSet[(attributes()+1)*attributes()];  //TODO: overkill for parallel versions..
		impl(X, Y, 0, implied);
		// may avoid freeing this in one-shot program
		// can't do this in parallel version, as this is only the first step
		// delete[] implied;
	}
public:
	void run(ExtendedState& state){
		impl(state.extent, state.intent, state.j, state.implied);
	}
};

using FCbO = RecursiveAlgorithm<GenericFCbO>;

using ParFCbO = ForkJoinAlgorithm<GenericFCbO, FCbO>;
