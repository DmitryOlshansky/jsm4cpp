/**
	Algorithms adapted for Boost::MPI	
*/
#pragma once
#include "algorithm.hpp"
#include <boost/mpi.hpp>

namespace mpi = boost::mpi;

template<class GenericAlgo>
class WaveFrontMPI: public Algorithm {
	void algorithm(){
		mpi::environment env;
  		mpi::communicator world;
		
		auto algo = this->fork<WaveFrontSingle<GenericAlgo>>();
		algo.rank(world.rank());
		algo.waveSize(world.size());
		algo.run();
	}
public:
	using Algorithm::Algorithm;
};

