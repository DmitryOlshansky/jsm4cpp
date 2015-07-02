#include "sets.hpp"

//Instantiate enough of differently sized BitVec's
template<> size_t BitVec<0>::words = 0;
template<> size_t BitVec<0>::length = 0;
template<> size_t BitVec<1>::words = 0;
template<> size_t BitVec<1>::length = 0;

size_t LinearSet::total;

