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
		{ "fcbo", &make<FCbO> },
		{ "inclose2", &make<InClose2> },
		{ "inclose3", &make<InClose3> },
	// parallel
		{ "pcbo", &make<ParCbO> },
		{ "pfcbo", &make<ParInClose2> },
		{ "pinclose2", &make<ParInClose2> },
	};
	for(auto& e : table){
		if(name == e.name)
			return e.factory();
	}
	return unique_ptr<Algorithm>(nullptr);
}
