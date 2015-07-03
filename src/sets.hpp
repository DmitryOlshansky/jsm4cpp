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

#define BIT ((size_t)1)
#define MASK (~(size_t)0)

inline size_t popcnt(size_t arg)
{
	size_t bits = 0;
	while(arg){
		arg = arg & (arg-1);
		bits ++;
	}
	return bits;
}


class LinearSet{
	static size_t total;
	vector<unsigned> attrs;
	bool full; // == true - means all ones (to avoid allocating the whole vector)
public:
	explicit LinearSet(bool full_=false):full(full_){}
	static void setSize(size_t size){
		total = size;
	}
	static LinearSet newEmpty(){
		return LinearSet(false);
	}
	static LinearSet newFull(){
		return LinearSet(true);	
	}

	template<class Fn>
	void each(Fn&& fn){
		if(full){
			for(unsigned i=0; i<total; i++)
				fn(i);
		}
		else
			for_each(attrs.begin(), attrs.end(), fn);
	}
	
	void clearAll(){
		full = false;
		attrs.clear();
	}

	void setAll(){
		full = true;
		attrs.clear();
	}

	bool null(){
		return attrs.size() == 0;
	}

	bool hasMoreThen(size_t items){
		return attrs.size() > items;
	}

	void add(size_t val){
		if(full)
			return;
		if(attrs.size() && attrs.back() < val){ // 100% of uses for extents in *CbO and *InClose2/3
			attrs.push_back(val);
		}
		else {
			auto it = lower_bound(attrs.begin(), attrs.end(), val);
			attrs.insert(it, val);
		}
	}
};

/**
	Bit-vector implementation of integer set concept.
	Operations:
		- add value
		- has value
		- for each attribute call functor
		- intersect and intersect up to attribute
		- equal and equal up to attribute
*/
template<int tag>
class BitVec{
	enum { WORD_SIZE = sizeof(size_t), BITS = WORD_SIZE * 8 };
	static size_t length;
	static size_t words;
	size_t* data;
	//
	explicit BitVec(size_t* ptr) : data(ptr){}


#if defined(USE_MALLOC_ALLOC)
	static size_t* alloc(){
		return (size_t*)malloc(words*sizeof(size_t));
	}
	static size_t* alloc_zero(){
		return (size_t*)calloc(words, WORD_SIZE);
	}
	static void dispose(size_t* p){
		free(p);
	}
	static void setPoolSize(){}
#elif defined(USE_NEW_ALLOC)
	static size_t* alloc(){
		return new size_t[words];
	}
	static size_t* alloc_zero(){
		size_t* p =  new size_t[words];
		memset(p, 0, words*WORD_SIZE);
		return p;
	}
	static void dispose(size_t* p){
		if(!p)
			return;
		delete[] p;
	}
	static void setPoolSize(){}
#elif defined(USE_SHARED_POOL_ALLOC) 
	static UniquePool pool;
	static mutex* mut;
	static size_t* alloc(){
		lock_guard<mutex> guard(*mut);
		return (size_t*)pool->malloc();
	}
	static size_t* alloc_zero(){
		size_t* p = alloc();
		memset(p, 0, words*WORD_SIZE);
		return p;
	}
	static void dispose(size_t* p){
		if(!p)
			return;
		lock_guard<mutex> guard(*mut);
		pool->free(p);
	}
	static void setPoolSize(){
		lock_guard<mutex> guard(*mut);
		pool = UniquePool(new Pool(words*WORD_SIZE));
	}
#elif defined(USE_TLS_POOL_ALLOC)
	static __thread Pool* pool;
	static size_t* alloc(){
		if(!pool)
			pool = new Pool(words*WORD_SIZE);
		return (size_t*)pool->malloc();
	}
	static size_t* alloc_zero(){
		size_t* p = alloc();
		memset(p, 0, words*WORD_SIZE);
		return p;
	}
	static void dispose(size_t* p){
		if(!p)
			return;
		pool->free(p);
	}
	static void setPoolSize(){}
#else
	#error "Must use some allocator for sets."
#endif

	//
public:
	// allocate array of sets, permanently (don't try to delete this array!)
	static BitVec* newArray(size_t n){
		BitVec* ptrs = new BitVec[n];
		size_t* p = (size_t*)calloc(n*words, WORD_SIZE);
		for (size_t i = 0; i < n; i++){
			ptrs[i] = BitVec(p + words*i);
		}
		return ptrs;
	}
	//
	static BitVec newEmpty(){
		return BitVec(alloc_zero());
	}
	//
	static BitVec newFull(){
		BitVec vec(alloc());
		vec.setAll();
		return vec;
	}
	//
	static void setSize(size_t total){
		length = total;
		words = (length + BITS - 1) / BITS;
		setPoolSize();
	}

	BitVec():data(nullptr){}

	BitVec(BitVec&& v){
		data = v.data;
		v.data = nullptr;
	}

	BitVec& operator=(BitVec&& v){
		BitVec::~BitVec();
		data = v.data;
		v.data = nullptr;
		return *this;
	}

	bool null() const {
		return data == nullptr;
	}

	void clearAll(){
		memset(data, 0, words*WORD_SIZE);
	}

	size_t hasMoreThen(size_t items){
		size_t so_far = 0;
		for (size_t i = 0; i < words; i++){
			if(data[i]){
				so_far += popcnt(data[i]);
				if(so_far > items)
					return true;
			}
		}
		return false;
	}

	void setAll(){
		// full words
		for (size_t i = 0; i < words; i++){
			data[i] = -1; // count on 2's complement arithmetic to get 0xFFFFFFFF...
		}
		// cut ones for the last word
		size_t tail = length % BITS;
		data[words - 1] &= (BIT << tail) - 1;
	}

	// apply to each item, calls functor with integers
	template<class Fn>
	void each(Fn&& functor){
		auto limit = words;
		for (size_t i = 0; i < limit; i++){
			if (data[i]){ // skip word at a time if empty
				for (size_t j = 0, m = 1<<j; j < BITS; j++, m<<= 1){
					if (data[i] & m)
						functor(i*BITS + j);
				}
			}
		}
	}

	// copy other set over this one
	void copy(BitVec& vec){
		memcpy(data, vec.data, words*WORD_SIZE);
	}

	// 
	bool equal(BitVec& vec, size_t up_to){
		size_t end = up_to / BITS;
		// full words
		for (size_t i = 0; i < end; i++){
			if (data[i] != vec.data[i])
				return false;
		}
		// handle last word
		size_t tail = up_to % BITS;
		if (!tail)
			return true;
		size_t mask = (BIT << tail) - 1;
		size_t mismatch = data[end] ^ vec.data[end];
		return (mismatch & mask) == 0;
	}

	//
	bool equal(BitVec& vec){
		return memcmp(&data[0], &vec.data[0], words*WORD_SIZE) == 0;
	}

	// set number j
	BitVec& add(size_t j){
		data[j / BITS] |= (BIT) << (j & MASK);
		return *this;
	}

	BitVec& remove(size_t j){
		data[j / BITS] &= ~((BIT) << (j & MASK));
		return *this;
	}

	// contains number j
	bool has(size_t j) {
		return (data[j / BITS] & (BIT << (j & MASK))) != 0;
	}

	BitVec& merge(BitVec& vec){
		for (size_t i = 0; i < words;i++){
			data[i] |= vec.data[i];
		}
		return *this;
	}

	BitVec& intersect(BitVec& vec){
		for (size_t i = 0; i < words;i++){
			data[i] &= vec.data[i];
		}
		return *this;
	}

	// intersect up to given attribute
	BitVec& intersect(BitVec& vec, size_t up_to){
		size_t end = up_to / BITS;
		for (size_t i = 0; i < end; i++){
			data[i] &= vec.data[i];
		}
		size_t tail = up_to % BITS;
		if (tail){
			size_t mask = (BIT << tail) - 1;
			data[end] = mask & data[end] & vec.data[end];
		}
		return *this;
	}

	bool subsetOf(BitVec& vec, size_t up_to){
		size_t end = up_to / BITS;
		for (size_t i = 0; i < end; i++){
			if (data[i] != (data[i] & vec.data[i]))
				return false;
		}
		size_t tail = up_to % BITS;
		if (tail){
			size_t mask = (BIT << tail) - 1;
			return (mask & data[end]) == (mask & data[end] & vec.data[end]);
		}
		return true;
	}

	~BitVec(){
		dispose(data);// dispose must be safe on nullptr
	}
};


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
