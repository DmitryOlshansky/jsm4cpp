/**

*/
#pragma once

#include "algorithm.hpp"

class InClose3 : virtual public HybridAlgorithm {
	using HybridAlgorithm::HybridAlgorithm;

	void inclose3Impl(ExtSet A, IntSet B, size_t y, IntSet* N){
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
			inclose3Impl(r.extent, r.intent, r.j + 1, M);
			q.pop();
		}
	}

	void algorithm(){
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet X = exts.newFull();
		IntSet Y = ints.newEmpty();
		// intents with implied error, see FCbO papper
		IntSet* implied = new IntSet[(attributes() + 1)*attributes()];
		inclose3Impl(X, Y, 0, implied);
	}
	/*

	void parInclose3Impl(ExtSet A, IntSet B, size_t y, IntSet* N, size_t rec_level){
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
				putToThread(r.extent, r.intent, r.j, M);
			}
			else
				parInclose3Impl(r.extent, r.intent, r.j + 1, M, rec_level+1);
			q.pop();
		}
	}

	void parInclose3(){
		threadBlks = new ThreadBlock[threads()];
		for (size_t i = 0; i < threads(); i++){
			threadBlks[i].implied = new IntSet[max((size_t)2, attributes() + 1 - parLevel())*attributes()];
			threadBlks[i].M = IntSet::newArray(attributes());
		}
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet X = exts.newFull();
		IntSet Y = ints.newEmpty();
		// intents with implied error, see FCbO papper
		IntSet* implied = new IntSet[(parLevel() + 2)*attributes()];
		parInclose3Impl(X, Y, 0, implied, 0);
		// start of multi-threaded part

		vector<thread> trds(threads());

		for (size_t t = 0; t < threads(); t++){
			trds[t] = thread([t, this]{
				size_t j;
				Algorithm c = *this; // use separate Algorithm to count operations
				memset(&c.stats, 0, sizeof(c.stats));
				while (!threadBlks[t].queue.empty()){
					threadBlks[t].queue.fetch(threadBlks[t].A, threadBlks[t].B, j, threadBlks[t].M, attributes());
					for (size_t i = 0; i < attributes(); i++){
						threadBlks[t].implied[i] = threadBlks[t].M[i];
					}
					c.inclose3Impl(threadBlks[t].A, threadBlks[t].B, j + 1, threadBlks[t].implied);
				}
			});
		}
		for (auto & t : trds){
			t.join();
		}
	}
	*/
};
