/**

*/
#pragma once

#include "algorithm.hpp"

class InClose3 : virtual public HybridAlgorithm {
	using HybridAlgorithm::HybridAlgorithm;

	void impl(ExtSet A, IntSet B, size_t y, IntSet* N){
		if (y == attributes()){
			output(A, B);
			return;
		}
		queue<Rec> q;
		IntSet::Pool ints(attributes() - y);
		ExtSet::Pool exts(attributes() - y);
		ExtSet C;
		IntSet D;
		IntSet* M = N + attributes();
		for (size_t j = y; j < attributes(); j++) {
			M[j] = N[j];
			if (!B.has(j)){
				if (N[j].empty() || N[j].subsetOf(B, j)){ // subset of (considering attributes < j)
					C = ExtSet::newEmpty();
					if (filterExtent(A, j, C)){ // if A == C
						B.add(j);
					}
					else{
						D = IntSet::newFull();
						partialClosure(C, j, D);
						if (B.equal(D, j)){ // equal up to <j
							q.emplace(C, D, j);
						}
						else {
							M[j] = D;
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
			Rec r = q.front();
			r.intent.copy(B);
			r.intent.add(r.j);
			impl(r.extent, r.intent, r.j + 1, M);
			q.pop();
		}
	}

	void algorithm(){
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet X = ExtSet::newFull();
		IntSet Y = IntSet::newEmpty();
		// intents with implied error, see FCbO papper
		IntSet* implied = new IntSet[(attributes() + 1)*attributes()];
		impl(X, Y, 0, implied);
	}
public:
	void run(ExtendedState& state){
		impl(state.extent, state.intent, state.j, state.implied);
	}
};

class ParInClose3: virtual public HybridAlgorithm, public ParallelAlgorithm<ExtendedState, InClose3> {
	using ParallelAlgorithm::ParallelAlgorithm;

	void impl(ExtSet A, IntSet B, size_t y, IntSet* N, size_t rec_level){
		if (y == attributes()){
			output(A, B);
			return;
		}
		queue<Rec> q;
		IntSet::Pool ints(attributes() - y);
		ExtSet::Pool exts(attributes() - y);
		ExtSet C;
		IntSet D;
		IntSet* M = N + attributes();
		for (size_t j = y; j < attributes(); j++) {
			M[j] = N[j];
			if (!B.has(j)){
				if (N[j].empty() || N[j].subsetOf(B, j)){ // subset of (considering attributes < j)
					C = ExtSet::newEmpty();
					if (filterExtent(A, j, C)){ // if A == C
						B.add(j);
					}
					else{
						D = IntSet::newFull();
						partialClosure(C, j, D);
						if (B.equal(D, j)){ // equal up to <j
							q.emplace(C, D, j);
						}
						else {
							M[j] = D;
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
			Rec r = q.front();
			r.intent.copy(B);
			r.intent.add(r.j);
			if (rec_level == parLevel()){
				memset(M, 0, sizeof(IntSet)* y); // clear first y IntSets that are possibly stale 
				schedule(ExtendedState{r.extent, r.intent, r.j + 1, M, attributes()});
			}
			else
				impl(r.extent, r.intent, r.j + 1, M, rec_level + 1);
			q.pop();
		}
	}

	void serialStep(){
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet X = ExtSet::newFull();
		IntSet Y = IntSet::newEmpty();
		// intents with implied error, see FCbO papper
		IntSet* implied = new IntSet[(parLevel() + 2)*attributes()];
		impl(X, Y, 0, implied, 0);
	}
};