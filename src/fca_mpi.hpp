/**
	Formal concepts analysis header.
*/

#pragma once

#include "algorithm_mpi.hpp"
#include "cbo.hpp"
#include "fcbo.hpp"
#include "inclose2.hpp"
#include "inclose3.hpp"

using MPI_WFCbO = WaveFrontMPI<GenericBCbO>;
using MPI_WFFCbO = WaveFrontMPI<GenericFCbO>;
using MPI_WFInClose2 = WaveFrontMPI<GenericInClose2>;
using MPI_WFInClose3 = WaveFrontMPI<GenericInClose3>;

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
	// wave-front parallel MPI-enabled
		{ "wf-bcbo", &make<MPI_WFCbO> },
		{ "wf-fcbo", &make<MPI_WFFCbO> },
		{ "wf-inclose2", &make<MPI_WFInClose2> },
		{ "wf-inclose3", &make<MPI_WFInClose3> }
	};
	for(auto& e : table){
		if(name == e.name)
			return e.factory();
	}
	return unique_ptr<Algorithm>(nullptr);
}
