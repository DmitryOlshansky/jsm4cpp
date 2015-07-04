#pragma once
#include <unordered_set>

class HashSet{
	static size_t total;
	unordered_set<unsigned> attrs;
	bool full; // == true - means all ones (to avoid allocating the whole hash table)
public:
	explicit HashSet(bool full_=false):full(full_){}
	static HashSet* newArray(size_t n){
		HashSet* ptrs = new HashSet[n];
		return ptrs;
	}
	static void setSize(size_t size){
		total = size;
	}
	static HashSet newEmpty(){
		return HashSet(false);
	}
	static HashSet newFull(){
		return HashSet(true);	
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

	bool has(size_t val){
		if(full)
			return true;
		if(attrs.size() == 0)
			return false;
		if(val < attrs.front())
			return false;
		if(val > attrs.back())
			return false;
		return binary_search(attrs.begin(), attrs.end(), val);
	}

	bool null(){
		return !full && attrs.size() == 0;
	}

	bool hasMoreThen(size_t items){
		return full || attrs.size() > items;
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

	void copy(HashSet& set){
		if(set.full){
			full = true;
			attrs.clear();
		}
		else{
			full = false;
			attrs.resize(set.attrs.size());
			std::copy(set.attrs.begin(), set.attrs.end(), attrs.begin());
		}
	}

	// 
	bool equal(HashSet& set, size_t up_to){
		if(full ^ set.full)
			return false;
		// here we got both full or both not full
		if(full)
			return true;
		auto tend = lower_bound(attrs.begin(), attrs.end(), up_to);
		auto set_end = lower_bound(set.attrs.begin(), set.attrs.end(), up_to);
		if(tend - attrs.begin() != set_end - set.attrs.begin())
			return false;
		return std::equal(attrs.begin(), tend, set.attrs.begin());
	}

	//
	bool equal(HashSet& set){
		if(full ^ set.full)
			return false;
		// here we got both full or both not full
		return full || attrs == set.attrs;
	}

	HashSet& intersect(HashSet& set){
		if(set.full){
			return *this;
		}
		if(full){ // full but the other one isn't
			attrs = set.attrs;
			full = false;
			return *this;
		}
		// both are not full - need to compute intersection
		vector<unsigned> inter;
		set_intersection(attrs.begin(), attrs.end(),
			set.attrs.begin(), set.attrs.end(), 
			std::back_inserter(inter));
		attrs = inter;
		return *this;
	}

	// intersect up to given attribute
	HashSet& intersect(HashSet& set, size_t up_to){
		if(set.full){
			return *this;
		}
		if(full){ // full but the other one isn't
			attrs = set.attrs;
			full = false;
			return *this;
		}
		// both are not full - need to compute intersection 
		vector<unsigned> inter;
		// minding the up_to
		auto tset = lower_bound(set.attrs.begin(), set.attrs.end(), up_to);
		set_intersection(attrs.begin(), attrs.end(),
			set.attrs.begin(), tset, 
			std::back_inserter(inter));
		attrs = inter;
		return *this;
	}

	bool subsetOf(HashSet& set, size_t up_to){
		if(full){
			if(set.full)
				return true;
			auto set_end = lower_bound(set.attrs.begin(), set.attrs.end(), up_to);
			return set_end - set.attrs.begin() == up_to; // must have all up_to of elements
		}
		if(set.full){ // same but reverse - now our set must have all of elements
			auto tend = lower_bound(attrs.begin(), attrs.end(), up_to);
			return tend - attrs.begin() == up_to; // must have all up_to of elements
		}
		auto tend = lower_bound(attrs.begin(), attrs.end(), up_to);
		auto set_end = lower_bound(set.attrs.begin(), set.attrs.end(), up_to);
		if(tend - attrs.begin() != set_end - set.attrs.begin()) // different size of ranges
			return false;
		// got to compare
		return std::equal(attrs.begin(), tend, set.attrs.begin());
	}
};
