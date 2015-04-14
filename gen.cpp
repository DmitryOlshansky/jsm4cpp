/*
jsm-generate driver, a part of JSM toolset
that does actual generation of concept lattice.

FIMI data set as input. 0-N integers

Input:
attributes of object as space separated integers \n
...

Output:
attributes of concept as integers \n

Authors: Dmitry Olshansky (c) 2015-
*/
#include <assert.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <utility>
#include <memory>

using namespace std;

/**
	Bit-vector implementation of integer set concept.
	Operations:
		- add value
		- has value
		- for each attribute call functor
		- intersect
	Movable, owns fixed chunk of memory.
*/
template<int tag>
class BitVec{
	enum { BYTES = sizeof(size_t), BITS = BYTES * 8 };
	const unsigned long long MASK = ~0ULL;
	static size_t length;
	static size_t words;
	unique_ptr<size_t[]> data;
public:
	static void setSize(size_t total){
		length = total;
		words = (length + BITS - 1) / BITS;
	}
	BitVec() : data(new size_t[words]()){}
	BitVec(BitVec&& bv) : data(move(bv.data)) {}

	void setAll(){
		// full words
		for (size_t i = 0; i < words; i++){
			data[i] = -1; // count on 2's complement arithmetic to get 0xFFFFFFFF...
		}
		// cut ones for the last word
		size_t tail = length % BITS;
		data[words - 1] &= (1ULL << tail) - 1;
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
		size_t mask = (1ULL << tail) - 1;
		size_t mismatch = data[end] ^ vec.data[end];
		return (mismatch & mask) == 0;
	}

	//
	bool equal(BitVec& vec){
		return memcmp(&data[0], &vec.data[0], words*BYTES) == 0;
	}

	// set number j
	void add(size_t j){
		data[j / BITS] |= (1ULL)<<(j & MASK);
	}

	// contains number j
	bool has(size_t j) {
		return (data[j / BITS] & (1ULL << (j & MASK))) != 0;
	}

	BitVec& intersect(BitVec& vec){
		for (size_t i = 0; i < words;i++){
			data[i] &= vec.data[i];
		}
		return *this;
	}

	BitVec& intersect(BitVec& vec, size_t up_to){
		size_t end = up_to / BITS;
		for (size_t i = 0; i < end; i++){
			data[i] &= vec.data[i];
		}
		size_t tail = up_to % BITS;
		if (tail){
			size_t mask = (1ULL << tail) - 1;
			data[end] = mask & data[end] & vec.data[end];
		}
		return *this;
	}
};

template<class Set>
void print(Set& set, ostream& sink=cerr){
	set.each([&](size_t i){
		sink << i << ' ';
	});
	sink << '\n';
}

/**
	Context is parametrized by 2 Set implementations.
	One is used for intents and the other for extents.
*/
template<class ExtSet, class IntSet>
class Context {
private:
	vector<IntSet> rows; // attributes of objects
	vector<size_t> supps; // support of each attribute
	size_t attributes;
	size_t objects;

	struct Rec{ // internal struct for enqueing (F)CbO and InClose calls 
		ExtSet extent;
		IntSet intent;
		size_t j;
		Rec(ExtSet e, IntSet i, size_t y) :extent(move(e)), intent(move(i)), j(y){}
		Rec(Rec&& r) : extent(move(r.extent)), intent(move(r.intent)), j(r.j){}
	};
public:
	Context() :rows(), attributes(0), objects(0){}

	// print intent and/or extent
	void output(ExtSet& A, IntSet& B){
		print(B, cout);
	}

	void printContext(){
		for (auto& obj : rows){
			print(obj);
		}
	}

	bool loadFIMI(istream& inp){
		vector<vector<int>> values;
		int max_attribute = -1;
		string s;
		while (getline(inp, s)){
			vector<int> vals;
			stringstream ss(s);
			for (;;){
				int val = -1;
				ss >> val;
				if (val == -1)
					break;
				if (max_attribute < val){
					max_attribute = val;
				}
				vals.push_back(val);
			}
			values.push_back(move(vals));
		}
		if (values.size() == 0){
			return false;
		}
		objects = values.size();
		attributes = max_attribute + 1;
		ExtSet::setSize(objects);
		IntSet::setSize(attributes);
		rows.resize(objects);
		supps.resize(attributes);
		for (size_t i = 0; i < objects; i++){
			for (auto val : values[i]){
				rows[i].add(val);
				supps[val]++;
			}
		}
		return true;
	}
	/**
		Inputs: A - extent, B - intent, y - new attribute
		Output: C = A intersect closure({j}); D = closure of C
	*/
	void closeConcept(ExtSet& A, size_t y, ExtSet& C, IntSet& D){
		D.setAll();
		A.each([&](size_t i){
			if (rows[i].has(y)){
				C.add(i);
				D.intersect(rows[i]);
			}
		});
	}
	
	// an interation of Close by One algorithm
	void cboImpl(ExtSet A, IntSet B, size_t y) {
		output(A, B);
		for (size_t j = y; j < attributes; j++) {
			if (!B.has(j)){
				ExtSet C;
				IntSet D;
				closeConcept(A, j, C, D);
				if (B.equal(D, j)){ // equal up to <j
					cboImpl(move(C), move(D), j + 1);
				}
			}
		}
	}

	void cbo(){
		ExtSet X;
		IntSet Y;
		X.setAll();
		Y.setAll();
		X.each([&](size_t i){
			Y.intersect(rows[i]);
		});
		cboImpl(move(X), move(Y), 0);
	}

	void fcboImpl(ExtSet A, IntSet B, size_t y, vector<IntSet> Ny){
		queue<Rec> q;
		output(A, B);
		for (size_t j = y; j < attributes; j++) {
			if (!B.has(j)){
				ExtSet C;
				IntSet D;
				closeConcept(A, j, C, D);
				if (B.equal(D, j)){ // equal up to <j
					q.emplace(move(C), move(D), j + 1);
				}
			}
		}
		while (!q.empty()){
			Rec r = move(q.front());
			fcboImpl(move(r.extent), move(r.intent), r.j, Ny); // HERE!
			q.pop();
		}
	}

	void fcbo(){
		ExtSet X;
		IntSet Y;
		X.setAll();
		Y.setAll();
		X.each([&](size_t i){
			Y.intersect(rows[i]);
		});
		vector<IntSet> Nj(attributes);
		fcboImpl(move(X), move(Y), 0, move(Nj));
	}
};

using OurContext = Context<BitVec<0>, BitVec<1>>;

// Sadly fixed BitVectors need separate internals declaration per instance
size_t BitVec<0>::words;
size_t BitVec<0>::length;
size_t BitVec<1>::words;
size_t BitVec<1>::length;

int main(int argc, char* argv[])
{
	OurContext context;
	istream& inp = cin;
	ostream& out = cout;
	if (argc >= 2){
		ifstream f(argv[1]);
		if (!context.loadFIMI(f)){
			cerr << "Failed to load any data.\n";
			return 1;
		}
	}
	else if (!context.loadFIMI(cin)){
		cerr << "Failed to load any data.\n";
		return 1;
	}
	context.printContext();

	context.fcbo();
	return 0;
}
