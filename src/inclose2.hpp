/**

*/
#pragma once

#include "algorithm.hpp"

class InClose2 : virtual public HybridAlgorithm {
	using HybridAlgorithm::HybridAlgorithm;

	void inclose2Impl(ExtSet A, IntSet B, size_t y){
		if (y == attributes()){
			output(A, B);
			return;
		}
		queue<Rec> q;
		IntSet::Pool ints(attributes() - y);
		ExtSet::Pool exts(attributes() - y);
		ExtSet C;
		IntSet D;
		for (size_t j = y; j < attributes(); j++) {
			if (!B.has(j)){
				C = exts.newEmpty();
				if (filterExtent(A, j, C)){ // if A == C
					B.add(j);
				}
				else{
					D = ints.newFull();
					partialClosure(C, j, D);
					if (B.equal(D, j)){ // equal up to <j
						q.emplace(C, D, j);
					}
					else
						stats.fail_canon++;
				}
			}
		}
		output(A, B);
		while (!q.empty()){
			Rec r = q.front();
			r.intent.copy(B);
			r.intent.add(r.j);
			inclose2Impl(r.extent, r.intent, r.j+1);
			q.pop();
		}
	}

	void algorithm(){
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet X = exts.newFull();
		IntSet Y = ints.newEmpty();
		inclose2Impl(X, Y, 0);
	}

	friend class ParInClose2;
};

class ParInClose2 : virtual public HybridAlgorithm, public ParallelAlgorithm<SimpleState> {
	using ParallelAlgorithm::ParallelAlgorithm;

	void parInclose2Impl(ExtSet A, IntSet B, size_t y, size_t rec_level){
		if (y == attributes()){
			output(A, B);
			return;
		}
		queue<Rec> q;
		IntSet::Pool ints(attributes() - y);
		ExtSet::Pool exts(attributes() - y);
		ExtSet C;
		IntSet D;
		for (size_t j = y; j < attributes(); j++) {
			if (!B.has(j)){
				C = exts.newEmpty();
				if (filterExtent(A, j, C)){ // if A == C
					B.add(j);
				}
				else{
					D = ints.newFull();
					partialClosure(C, j, D);
					if (B.equal(D, j)){ // equal up to <j
						q.emplace(C, D, j);
					}
					else
						stats.fail_canon++;
				}
			}
		}
		output(A, B);
		while (!q.empty()){
			Rec r = q.front();
			r.intent.copy(B);
			r.intent.add(r.j);
			if (rec_level == parLevel())
				schedule(SimpleState{r.extent, r.intent, r.j + 1});
			else
				parInclose2Impl(r.extent, r.intent, r.j + 1, rec_level+1);
			q.pop();
		}
	}

	void serialStep(){
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet X = exts.newFull();
		IntSet Y = ints.newEmpty();
		parInclose2Impl(X, Y, 0, 0);
	}

	void parallelStep(size_t tid){
		SimpleState state;
		ExtSet::Pool extP(1);
		IntSet::Pool intP(1);
		state.extent = extP.newEmpty();
		state.intent = intP.newEmpty();
		auto sub = fork<InClose2>();
		memset(&sub.stats, 0, sizeof(sub.stats));
		while (extract(tid, state)){
			sub.inclose2Impl(state.extent, state.intent, state.j);
		}
	}
};