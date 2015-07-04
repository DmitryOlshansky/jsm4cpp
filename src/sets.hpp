/**
	Multiple SET datastructures for use in FCA concept generation,
	frequent itemset mining or JSM induction step.

	BitVec - fixed-length bitvector, length is static and must be set before use
	LinearSet - ordered array of integers
	StdSet - B-Tree based on C++11 set 
	HashSet - hash table based on C++11 unordered_set 
*/
#pragma once

#include <assert.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <ostream>
#include <memory>

#if defined(USE_SHARED_POOL_ALLOC) || defined(USE_TLS_POOL_ALLOC)
	#include <mutex>
	#include <boost/pool/pool.hpp>
	using Pool = boost::pool<>;
	using UniquePool = std::unique_ptr<Pool>;
#endif

using namespace std;

#include "bitvec.hpp"
#include "linear_set.hpp"
#include "tree.hpp"
//#include "hash_set.hpp"

template<class Set>
ostream& printSet(Set& set, ostream& os){
	bool first = true;
	set.each([&](size_t v){
		if (first)
			first = false;
		else
			os<< ' ';
		os<<v;
	});
	return os;
}

// Helper - set to zeros, optionally allocating data if it was null
template<class Set>
Set& toEmpty(Set& s){
	if(s.null())
		s = Set::newEmpty();
	else
		s.clearAll();
	return s;
}

// Helper - set to ones, optionally allocating data if it was null
template<class Set>
Set& toFull(Set& s){
	if(s.null())
		s = Set::newFull();
	else
		s.setAll();
	return s;
}


// Compressed set is either a reference to some existing set
// or a set itself; this transparently reuses existing sets w/o copying
// while keeping the ability to replace it later with proper set.
template<class Set>
class CompressedSet{
	union{
		Set set;
		Set* link;
	};
	// empty sets dominate, so cache emptiness
	// need not to worry about linked "owner" as we never change a layer
	// above the current one (~ linked set is effectively immutable )
	bool is_link, is_null;
public:
	CompressedSet():set(),is_link(false),is_null(true){}
	CompressedSet(Set&& val):set(val),is_link(false),is_null(val.null()){}
	explicit CompressedSet(Set* ptr):link(ptr), is_link(true){}
	// link to the other compressed set
	CompressedSet& operator=(CompressedSet& cs){
		if(!is_link) // destroy our set if stand-alone
			set.~Set();
		if(cs.is_link){
			link = cs.link;
		}
		else{
			link = &cs.set;
		}
		is_null = cs.is_null;
		is_link = true;

		return *this;
	}
	bool null(){ return is_null; }
	CompressedSet& operator=(Set&& val){
		if(!is_link){
			set.~Set();
		}
		new (&set) Set(move(val));
		is_link = false;
		is_null = set.null();
		return *this;
	}
	Set* operator ->(){ return is_link ? link : &set; }
	~CompressedSet(){
		if(!is_link){
			set.~Set();
		}
	}
};
