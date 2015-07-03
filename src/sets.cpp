#include "sets.hpp"

//Instantiate enough of differently sized BitVec's
template<> size_t BitVec<0>::words = 0;
template<> size_t BitVec<0>::length = 0;
template<> size_t BitVec<1>::words = 0;
template<> size_t BitVec<1>::length = 0;

#if defined(USE_SHARED_POOL_ALLOC)
	template<> UniquePool BitVec<0>::pool(nullptr);
	template<> mutex* BitVec<0>::mut = new mutex;
	template<> UniquePool BitVec<1>::pool(nullptr);
	template<> mutex* BitVec<1>::mut = new mutex;
#elif defined(USE_TLS_POOL_ALLOC)
	template<> __thread Pool* BitVec<0>::pool = nullptr;
	template<> __thread Pool* BitVec<1>::pool = nullptr;
#endif
size_t LinearSet::total;

