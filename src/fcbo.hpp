/**

*/
#pragma once

#include "algorithm.hpp"

class FCbO : virtual public HybridAlgorithm {
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
				if (N[j]->null() || N[j]->subsetOf(B, j)){ // subset of (considering attributes < j)
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
			impl(r.extent, r.intent, r.j+1, M);
			q.pop();
		}
	}

	void algorithm(){
		ExtSet X = ExtSet::newFull();
		IntSet Y = IntSet::newFull();
		X.each([&](size_t i){
			Y.intersect(row(i));
		});
		// intents with implied error, see FCbO papper
		CompIntSet* implied = new CompIntSet[(attributes()+1)*attributes()];
		impl(X, Y, 0, implied);
	}
public:
	void run(ExtendedState& state){
		impl(state.extent, state.intent, state.j, state.implied);
	}
};
/*
class ParFCbO : virtual public HybridAlgorithm, public ParallelAlgorithm<ExtendedState, FCbO>{
	using ParallelAlgorithm::ParallelAlgorithm;

	void parFcboImpl(ExtSet A, IntSet B, size_t y, IntSet* N, size_t rec_level){
		output(A, B);
		if (y == attributes())
			return;
		queue<Rec> q;
		IntSet* M = N + attributes();
		IntSet::Pool ints(attributes() - y);
		ExtSet::Pool exts(attributes() - y);
		ExtSet C;
		IntSet D;
		for (size_t j = y; j < attributes(); j++) {
			M[j] = N[j];
			if (!B.has(j)){
				if (N[j].empty() || N[j].subsetOf(B, j)){ // subset of (considering attributes < j)
					// C empty, D full is a precondition
					C = ExtSet::newEmpty();
					D = IntSet::newFull();
					if(closeConcept(A, j, C, D)){
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
		while (!q.empty()){
			Rec r = q.front();
			if (rec_level == parLevel()){
				memset(M, 0, sizeof(IntSet) * y); // clear first y IntSets that are possibly stale 
				schedule(ExtendedState{r.extent, r.intent, r.j + 1, M, attributes()});
			}
			else{
				parFcboImpl(r.extent, r.intent, r.j + 1, M, rec_level+1);
			}
			q.pop();
		}
	}

	void serialStep(){
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet X = ExtSet::newFull();
		IntSet Y = IntSet::newFull();
		X.each([&](size_t i){
			Y.intersect(row(i));
		});
		// intents with implied error, see FCbO papper
		IntSet* implied = new IntSet[(parLevel()+2)*attributes()];
		parFcboImpl(X, Y, 0, implied, 0);
	}
};
*/