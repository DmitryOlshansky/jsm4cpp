/**
	Различные реализации множеств для применения в JSM процедурах.

	LinearSet - сортированный массив целых чисел
	BitVec - битовый вектор фиксированной длины
*/
#pragma once

#include <assert.h>
#include <cstdlib>
#include <cstring>
#include <vector>


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

struct LinearSet{
	struct Pool{
		Pool(size_t n):allocs(new LinearSet[n]), size(n), cur(0){}
		LinearSet newEmpty(){
			LinearSet ret = allocs[cur];
			ret.nums = new vector<int>();
			cur++;
			return ret;
		}
		LinearSet newFull(){
			LinearSet ret = allocs[cur];
			ret.nums = new vector<int>(total);
			for(size_t i=0; i<total; i++){
				(*ret.nums)[i] = i;
			}
			cur++;
			return ret;	
		}
		~Pool(){
			for(size_t i=0; i<size; i++){
				if(allocs[i].nums)
					delete allocs[i].nums;
			}
			delete[] allocs;
		}
		LinearSet* allocs;
		size_t size;
		size_t cur;
	};

	LinearSet():nums(nullptr){}

	template<class Fn>
	void each(Fn&& fn){
		for_each(nums->begin(), nums->end(), fn);
	}
	
	void clearAll(){
		nums->clear();
	}

	vector<int>* nums;

	size_t raw_size(){
		return nums->size()*sizeof(int) + sizeof(int);
	}

	bool hasMoreThen(size_t items){
		return nums->size() > items;
	}

	void add(size_t val){
		if(nums->size() && nums->back() < val)
			nums->push_back(val);
		else {
			auto it = lower_bound(nums->begin(), nums->end(), val);
			nums->insert(it, val);
		}
	}

	void store(void* ptr){
		int sz = (int)nums->size();
		memcpy(ptr, &sz, sizeof(int));
		memcpy((char*)ptr+sizeof(int), &(*nums)[0], sizeof(int)*nums->size());
	}

	void load(void* ptr){
		int sz;
		memcpy(&sz, ptr, sizeof(int));
		nums->resize(sz);
		memcpy(&(*nums)[0], (char*)ptr+sizeof(int), sizeof(int)*nums->size());
	}

	static void setSize(size_t size){
		total = size;
	}
	static size_t total;
};

size_t LinearSet::total;
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
	enum { BYTES = sizeof(size_t), BITS = BYTES * 8 };
	static size_t length;
	static size_t words;
	size_t* data;
public:
	// allocate array of sets, permanently
	static BitVec* newArray(size_t n){
		BitVec* ptrs = new BitVec[n];
		size_t* p = (size_t*)malloc(n*words*BYTES);
		for (size_t i = 0; i < n; i++){
			ptrs[i] = BitVec(p + words*i);
		}
		return ptrs;
	}
	// Fixed sized stack allocator for BitSet.
	// Frees all memory at once on scope exit.
	struct Pool{
		Pool(size_t n) : ptr((size_t*)malloc(n*words*BYTES)), cur(0), size(n*words){}
		BitVec newEmpty(){
			BitVec bv(ptr + cur);
			bv.clearAll();
			cur += words;
			assert(cur <= size);
			return bv;
		}
		BitVec newFull(){
			BitVec bv(ptr + cur);
			bv.setAll();
			cur += words;
			assert(cur <= size);
			return bv;
		}
		~Pool(){
			free(ptr);
		}
	private:
		size_t* ptr;
		size_t cur, size;
	};
	static void setSize(size_t total){
		length = total;
		words = (length + BITS - 1) / BITS;
	}

	BitVec():data(nullptr){}
	explicit BitVec(size_t* ptr) : data(ptr){}
	BitVec& operator=(const BitVec& v){
		data = v.data;
		return *this;
	}

	bool empty(){
		return data == nullptr;
	}

	void clearAll(){
		memset(data, 0, words*BYTES);
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
		memcpy(data, vec.data, words*BYTES);
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
		return memcmp(&data[0], &vec.data[0], words*BYTES) == 0;
	}

	// set number j
	void add(size_t j){
		data[j / BITS] |= (BIT) << (j & MASK);
	}

	// contains number j
	bool has(size_t j) {
		return (data[j / BITS] & (BIT << (j & MASK))) != 0;
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

	// operations to save in plain memory chunks
	static size_t raw_size(){
		return words*BYTES;
	}

	void store(void* ptr ){
		if (!data)
			memset(ptr, 0, words*BYTES);
		else
			memcpy(ptr, data, words*BYTES);
	}

	void load(void* ptr){
		memcpy(data, ptr, words*BYTES);
	}

	template<class Set>
	static bool incCanonical(Set objs, size_t j, BitVec* rows, BitVec B, BitVec D)
	{
		size_t end = j/BITS;
		for(size_t i=0; i<end; i++){
			size_t invb = ~B.data[i];
			size_t d = MASK;
			objs.each([&d, rows, i](size_t k){
				d &= rows[k].data[i];
			});
			if(d & invb)
				return false;
			D.data[i] = d;
		}
		size_t tail = j % BITS;
		if (tail){
			size_t invb = ~B.data[end];
			size_t d = (BIT << tail) - 1;
			objs.each([&d, rows, end](size_t k){
				d &= rows[k].data[end];
			});
			D.data[end] = d;
			if(d & invb)
				return false;
		}
		return true;
	}
};
