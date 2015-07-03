/**
	Formal concepts analysis header.
*/

#pragma once

#include "algorithm.hpp"
#include "cbo.hpp"
#include "fcbo.hpp"
#include "inclose2.hpp"
#include "inclose3.hpp"

template<class Algo>
unique_ptr<Algorithm> make(){
	return unique_ptr<Algorithm>(new Algo);
}

// Factory for Algorithms
inline unique_ptr<Algorithm> fromName(const string& name){
	struct Entry{
		const char* name;
		unique_ptr<Algorithm> (*factory)();
	};

	static vector<Entry> table = {
	// serial
		{ "cbo", &make<CbO> },
		{ "bcbo", &make<BCbO> },
		{ "fcbo", &make<FCbO> },
		{ "inclose2", &make<InClose2> },
		{ "inclose3", &make<InClose3> },
	// parallel: fork-join model based on ideas from P(F)CbO papers
		{ "p-bcbo", &make<ParCbO> },
		{ "p-fcbo", &make<ParFCbO> },
		{ "p-inclose2", &make<ParInClose2> },
		{ "p-inclose3", &make<ParInClose3> },
	// fair queue + fork-join
		{ "fp-bcbo", &make<FParCbO> },
		{ "fp-fcbo", &make<FParFCbO> },
		{ "fp-inclose2", &make<FParInClose2> },
		{ "fp-inclose3", &make<FParInClose3> },
	// fair queue + fork-join
		{ "tp-bcbo", &make<TPCbO> },
		{ "tp-fcbo", &make<TPFCbO> },
		{ "tp-inclose2", &make<TPInClose2> },
		{ "tp-inclose3", &make<TPInClose3> }
	};
	for(auto& e : table){
		if(name == e.name)
			return e.factory();
	}
	return unique_ptr<Algorithm>(nullptr);
}
