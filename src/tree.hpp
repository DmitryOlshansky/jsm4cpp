#pragma once

#include <iterator> 
#include <set>

#include "misc.hpp" // myEqual

class TreeSet{
	set<unsigned> tree;
	bool full;
	static size_t total;
	//
	bool hasAllUpTo(size_t up_to){
		auto tend = lower_bound(tree.begin(), tree.end(), up_to);
		size_t cnt = 0;
		for(auto it = tree.begin(); it != tend; it++)
			if(*it != cnt++)
				return false;
		return cnt == up_to; // must visit each integer in the range
	}
	//
public:
	// allocate array of sets, permanently (don't try to delete this array!)
	static TreeSet* newArray(size_t n){
		TreeSet* ptrs = new TreeSet[n];
		return ptrs;
	}
	static void setSize(size_t size){
		total = size;
	}
	static TreeSet newEmpty(){
		return TreeSet();
	}
	static TreeSet newFull(){
		return TreeSet(true);	
	}
	explicit TreeSet(bool is_full=false) : full(is_full){}

	bool null() const {
		return !full && tree.empty();
	}

	void clearAll(){
		full = false;
		tree.clear();
	}

	void setAll(){
		full = true;
		tree.clear();
	}

	// apply to each item, calls functor with integers
	template<class Fn>
	void each(Fn&& functor){
		if(full){
			for(size_t i=0; i<total; i++)
				functor(i);
		}
		else
			for_each(tree.begin(), tree.end(), functor);
	}

	// copy other set over this one
	void copy(TreeSet& set){
		full = set.full;
		tree = set.tree;
	}

	// 
	bool equal(TreeSet& set, size_t up_to){
		if(full && set.full){
			return true;
		}
		if(set.full)
			return hasAllUpTo(up_to);
		if(full)
			return set.hasAllUpTo(up_to);
		auto tend = lower_bound(tree.begin(), tree.end(), up_to);
		auto set_end = lower_bound(set.tree.begin(), set.tree.end(), up_to);
		return myEqual(tree.begin(), tend, set.tree.begin(), set_end);
	}

	//
	bool equal(TreeSet& set){
		if(full ^ set.full)
			return false;
		// here we got both full or both not full
		return full || tree == set.tree;
	}

	// set number j
	TreeSet& add(size_t j){
		if(!full){
			tree.insert(j);
		}
		return *this;
	}

	TreeSet& remove(size_t j){
		//FIXME: implement remove for full set
		assert(!full);
		tree.erase(j);
		return *this;
	}

	// contains number j
	bool has(size_t j) {
		return full || tree.find(j) != tree.end();
	}

	TreeSet& intersect(TreeSet& set){
		if(set.full){
			return *this;
		}
		if(full){ // full but the other one isn't
			tree = set.tree;
			full = false;
			return *this;
		}
		// both are not full - need to compute intersection
		std::set<unsigned> inter;
		set_intersection(tree.begin(), tree.end(),
			set.tree.begin(), set.tree.end(), 
			std::inserter(inter, inter.end()));
		tree = inter;
		return *this;
	}

	// intersect up to given attribute
	TreeSet& intersect(TreeSet& set, size_t up_to){
		if(set.full){
			return *this;
		}
		if(full){ // full but the other one isn't
			tree = set.tree;
			full = false;
			return *this;
		}
		// both are not full - need to compute intersection 
		std::set<unsigned> inter;
		// minding the up_to
		auto tset = lower_bound(set.tree.begin(), set.tree.end(), up_to);
		set_intersection(tree.begin(), tree.end(),
			set.tree.begin(), tset, 
			std::inserter(inter, inter.end()));
		tree = inter;
		return *this;
	}

	bool subsetOf(TreeSet& set, size_t up_to){
		if(full){
			if(set.full)
				return true;
			return set.hasAllUpTo(up_to);
		}
		if(set.full){ // same but reverse - now our set must have all of elements
			return hasAllUpTo(up_to);
		}
		auto tend = lower_bound(tree.begin(), tree.end(), up_to);
		auto set_end = lower_bound(set.tree.begin(), set.tree.end(), up_to);
		// got to compare
		return myEqual(tree.begin(), tend, set.tree.begin(), set_end);
	}
};
