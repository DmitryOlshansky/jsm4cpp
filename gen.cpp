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
#include <algorithm>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <utility>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>

using namespace std;

#define BIT ((size_t)1)
#define MASK (~(size_t)0)


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

/**
	Context is parametrized by 2 Set implementations.
	One is used for intents and the other for extents.
*/

using ExtSet = BitVec<0>;
using IntSet = BitVec<1>;


// Sadly fixed BitVectors need separate statics declaration per instance
template<> size_t BitVec<0>::words = 0;
template<> size_t BitVec<0>::length = 0;
template<> size_t BitVec<1>::words = 0;
template<> size_t BitVec<1>::length = 0;

// A per thread queue that contains flattened copy of required data
struct BlockQueue{
	BlockQueue(size_t initial){
		size = initial;
		cur_w = 0;
		cur_r = 0;
		data = (char*)malloc(initial);
	}

	void put(ExtSet extent, IntSet intent, size_t j){
		size_t to_write = extent.raw_size() + intent.raw_size() + sizeof(size_t);
		grow(cur_w + to_write);
		extent.store(data + cur_w);
		cur_w += extent.raw_size();
		intent.store(data + cur_w);
		cur_w += intent.raw_size();
		memcpy(data + cur_w, &j, sizeof(size_t));
		cur_w += sizeof(size_t);
	}

	void put(ExtSet extent, IntSet intent, size_t j, IntSet* M, size_t attrs){
		size_t to_write = extent.raw_size() + intent.raw_size() + sizeof(size_t) + intent.raw_size()*attrs;
		grow(cur_w + to_write);
		extent.store(data + cur_w);
		cur_w += extent.raw_size();
		intent.store(data + cur_w);
		cur_w += intent.raw_size();
		memcpy(data + cur_w, &j, sizeof(size_t));
		cur_w += sizeof(size_t);
		for (size_t i = 0; i < attrs; i++){
			M[i].store(data + cur_w);
			cur_w += M[i].raw_size();
		}
	}

	void fetch(ExtSet& extent, IntSet& intent, size_t& j){
		extent.load(data + cur_r);
		cur_r += extent.raw_size();
		intent.load(data + cur_r);
		cur_r += intent.raw_size();
		memcpy(&j, data + cur_r, sizeof(size_t));
		cur_r += sizeof(size_t);
	}

	void fetch(ExtSet& extent, IntSet& intent, size_t& j, IntSet* M, size_t attrs){
		fetch(extent, intent, j);
		for (size_t i = 0; i<attrs; i++){
			M[i].load(data + cur_r);
			cur_r += M[i].raw_size();
		}
	}

	bool empty(){
		return cur_r == cur_w;
	}

	void grow(size_t needed){
		if (needed > size){
			size = max(size * 2, needed);
			data = (char*)realloc(data, size);			
		}
	}
	char* data;
	size_t cur_w, size;
	size_t cur_r;
};


mutex output_mtx;

class Context {
private:
	enum { QUEUE_SIZE = 16 * 1024 };
	IntSet* rows; // attributes of objects
	size_t attributes;
	size_t objects;
	size_t* attributesNums; // sorted positions of attributes
	ostream* out;
	//
	size_t verbose;
	size_t par_level;
	size_t num_threads;
	
	struct ThreadBlock{
		BlockQueue queue;
		ExtSet::Pool aPool; 
		IntSet::Pool bPool;
		ExtSet A;
		IntSet B;
		IntSet* implied; // if has fast test, stack of (attributes - par_level)*attributes pointers
		IntSet* M; // if has fast test, real flat array of intents used to unpack from queue
		ThreadBlock() :queue(QUEUE_SIZE), aPool(1), bPool(1){
			A = aPool.newEmpty();
			B = bPool.newEmpty();
		}
	};

	ThreadBlock* threads;

	struct Stats{
		int total;
		int closures; // closures / partial closures computed
		int fail_canon; // canonical test failures
		int fail_fast; // fast canonical test failures
		Stats(): total(0), closures(0), fail_canon(0), fail_fast(0){}
	};
	Stats stats;

	struct Rec{ // internal struct for enqueing (F)CbO and InClose calls 
		ExtSet extent;
		IntSet intent;
		size_t j;
		Rec(ExtSet e, IntSet i, size_t y) :extent(e), intent(i), j(y){}
	};
	struct InRec{
		ExtSet extent;
		size_t j;
		InRec(ExtSet e, size_t y) :extent(e), j(y){}
	};

	
	void printAttributes(IntSet& set, ostream& sink = cerr){
		if (verbose >= 1){
			lock_guard<mutex> lock(output_mtx);
			set.each([&](size_t i){
				sink << attributesNums[i] << ' ';
			});
			sink << '\n';
		}
		stats.total++;
	}

	void printStats(){
		if (verbose >= 2){
			lock_guard<mutex> lock(output_mtx);
			cerr << "Total\tClosure\tCanonical\tFast\n"
			     << stats.total << '\t'<< stats.closures 
				 << '\t' << stats.fail_canon << '\t' << stats.fail_fast << endl;
		}
	}

	void putToThread(ExtSet extent, IntSet intent, size_t j, IntSet* M){
		static int tid = 0;
		threads[tid].queue.put(extent, intent, j, M, attributes);
		tid += 1;
		if (tid == num_threads)
			tid = 0;
	}
	void putToThread(ExtSet extent, IntSet intent, size_t j){
		static int tid = 0;
		threads[tid].queue.put(extent, intent, j);
		tid += 1;
		if (tid == num_threads)
			tid = 0;
	}
public:
	Context(size_t verbose_, size_t threads_, size_t par_level_) 
		:rows(), attributes(0), objects(0), out(&cout), verbose(verbose_), num_threads(threads_), par_level(par_level_){}
	~Context(){
		printStats();
	}
	void setOutput(ostream& sink){
		out = &sink;
	}

	// print intent and/or extent
	void output(ExtSet A, IntSet B){
		if(verbose >= 1)
			printAttributes(B, *out);
	}

	void printContext(){
		for (size_t i = 0; i < objects; i++){
			printAttributes(rows[i]);
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
		
		// Count supports and sort context
		// May also cut off attributes based on minimal support here and resize accordingly
		vector<size_t> supps(attributes);
		fill(supps.begin(), supps.end(), 0);

		attributesNums = new size_t[attributes];
		for (size_t i = 0; i < attributes; i++){
			attributesNums[i] = i;
		}

		for (size_t i = 0; i < objects; i++){
			for (auto val : values[i]){
				supps[val]++;
			}
		}

		// attributeNums[0] --> least frequent attribute num
		sort(attributesNums, attributesNums+attributes, [&](size_t i, size_t j){
			return supps[i] < supps[j];
		});
		
		vector<size_t> revMapping(attributes); // from original to sorted (most frequent --> 0)
		for (size_t i = 0; i < attributes; i++){
			revMapping[attributesNums[i]] = i;
		}

		// Setup context with re-ordered attributes
		ExtSet::setSize(objects);
		IntSet::setSize(attributes);
		rows = IntSet::newArray(objects);
		for (size_t i = 0; i < objects; i++){
			rows[i].clearAll();
			for (auto val : values[i]){
				rows[i].add(revMapping[val]);
			}
		}
		return true;
	}

	/**
		Inputs: A - extent, B - intent, y - new attribute
		C - empty, D - full
		Output: C = A intersect closure({j}); D = closure of C
	*/
	void closeConcept(ExtSet A, size_t y, ExtSet C, IntSet D){
		A.each([&](size_t i){
			if (rows[i].has(y)){
				C.add(i);
				D.intersect(rows[i]);
			}
		});
		stats.closures++;
	}

	// Produce extent having attribute y from A
	// true - if produced extent is identical
	bool filterExtent(ExtSet A, size_t y, ExtSet C){
		bool ret = true;
		//C.clearAll();
		A.each([&](size_t i){
			if (rows[i].has(y)){
				C.add(i);
			}
			else
				ret = false;
		});
		return ret;
	}

	// Close intent over extent C, up to y 
	void partialClosure(ExtSet C, size_t y, IntSet D){
		//D.setAll();
		C.each([&](size_t i){
			D.intersect(rows[i], y);
		});
		stats.closures++;
	}

	// an interation of Close by One algorithm
	void cboImpl(ExtSet A, IntSet B, size_t y) {
		output(A, B);
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet C = exts.newEmpty();
		IntSet D = ints.newFull();
		for (size_t j = y; j < attributes; j++) {
			if (!B.has(j)){
				// C empty, D full is a precondition
				closeConcept(A, j, C, D);
				if (B.equal(D, j)){ // equal up to <j
					cboImpl(C, D, j + 1);
				}
				else{
					stats.fail_canon++;
				}
				C.clearAll();
				D.setAll();
			}
		}
	}

	void cbo(){
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet X = exts.newFull();
		IntSet Y = ints.newFull();
		X.each([&](size_t i){
			Y.intersect(rows[i]);
		});
		cboImpl(X, Y, 0);
	}

	void fcboImpl(ExtSet A, IntSet B, size_t y, IntSet* N){
		output(A, B);
		if (y == attributes)
			return;
		queue<Rec> q;
		IntSet* M = N + attributes;
		IntSet::Pool ints(attributes - y);
		ExtSet::Pool exts(attributes - y);
		ExtSet C;
		IntSet D;
		// note that M[0..y] may point to stale sets which however doesn't matter
		for (size_t j = y; j < attributes; j++) {
			M[j] = N[j];
			if (!B.has(j)){
				if (N[j].empty() || N[j].subsetOf(B, j)){ // subset of (considering attributes < j)
					// C empty, D full is a precondition
					C = exts.newEmpty();
					D = ints.newFull();
					closeConcept(A, j, C, D);
					if (B.equal(D, j)){ // equal up to <j
						q.emplace(C, D, j);
					}
					else {
						stats.fail_canon++;
						M[j] = D;
					}
				}
				else
					stats.fail_fast++;
			}
		}
		
		while (!q.empty()){
			Rec r = q.front();
			fcboImpl(r.extent, r.intent, r.j+1, M);
			q.pop();
		}
	}

	void parFcboImpl(ExtSet A, IntSet B, size_t y, IntSet* N, size_t rec_level){
		output(A, B);
		if (y == attributes)
			return;
		queue<Rec> q;
		IntSet* M = N + attributes;
		IntSet::Pool ints(attributes - y);
		ExtSet::Pool exts(attributes - y);
		ExtSet C;
		IntSet D;
		for (size_t j = y; j < attributes; j++) {
			M[j] = N[j];
			if (!B.has(j)){
				if (N[j].empty() || N[j].subsetOf(B, j)){ // subset of (considering attributes < j)
					// C empty, D full is a precondition
					C = exts.newEmpty();
					D = ints.newFull();
					closeConcept(A, j, C, D);
					if (B.equal(D, j)){ // equal up to <j
						q.emplace(C, D, j);
					}
					else {
						M[j] = D;
						stats.fail_canon++;
					}
				}
				else
					stats.fail_fast++;
			}
		}
		while (!q.empty()){
			Rec r = q.front();
			if (rec_level == par_level){
				memset(M, 0, sizeof(IntSet) * y); // clear first y IntSets that are possibly stale 
				putToThread(r.extent, r.intent, r.j, M);
			}
			else{
				parFcboImpl(r.extent, r.intent, r.j + 1, M, rec_level+1);
			}
			q.pop();
		}
	}

	void parFcbo(){
		threads = new ThreadBlock[num_threads];
		for (size_t i = 0; i < num_threads; i++){
			threads[i].implied = new IntSet[max((size_t)2, attributes + 1 - par_level)*attributes]; 
			threads[i].M = IntSet::newArray(attributes);
		}
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet X = exts.newFull();
		IntSet Y = ints.newFull();
		X.each([&](size_t i){
			Y.intersect(rows[i]);
		});
		// intents with implied error, see FCbO papper
		IntSet* implied = new IntSet[(par_level + 2)*attributes];
		parFcboImpl(X, Y, 0, implied, 0);
		// start of multi-threaded part
		
		vector<thread> trds(num_threads);
		
		for (size_t t = 0; t < num_threads; t++){
			trds[t] = thread([t, this]{
				size_t j;
				Context c = *this; // use separate context to count operations
				ThreadBlock tb = threads[t];
				memset(&c.stats, 0, sizeof(c.stats));
				while (!tb.queue.empty()){
					tb.queue.fetch(tb.A, tb.B, j, tb.M, attributes);
					for (size_t i = 0; i < attributes; i++){
						tb.implied[i] = tb.M[i];
					}
					c.fcboImpl(tb.A, tb.B, j + 1, tb.implied);
				}
			});
		}
		for (auto & t : trds){
			t.join();
		}
	}

	void fcbo(){
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet X = exts.newFull();
		IntSet Y = ints.newFull();
		X.each([&](size_t i){
			Y.intersect(rows[i]);
		});
		// intents with implied error, see FCbO papper
		IntSet* implied = new IntSet[(attributes+1)*attributes];
		fcboImpl(X, Y, 0, implied);
	}

	void inclose2Impl(ExtSet A, IntSet B, size_t y){
		if (y == attributes){
			output(A, B);
			return;
		}
		queue<Rec> q;
		IntSet::Pool ints(attributes - y);
		ExtSet::Pool exts(attributes - y);
		ExtSet C;
		IntSet D;
		for (size_t j = y; j < attributes; j++) {
			if (!B.has(j)){
				C = exts.newEmpty();
				if (filterExtent(A, j, C)){ // if A == C
					B.add(j);
				}
				else{
					D = ints.newFull();
					if(IntSet::incCanonical(C, j, rows, B, D)){
					//if (B.equal(D, j)){ // equal up to <j
						q.emplace(C, D, j);
					}
					else
						stats.fail_canon++;
				}
			}
		}
		output(A, B);
		while (!q.empty()){
			Rec r = q.front();
			r.intent.copy(B);
			r.intent.add(r.j);
			inclose2Impl(r.extent, r.intent, r.j+1);
			q.pop();
		}
	}

	void parInclose2Impl(ExtSet A, IntSet B, size_t y, size_t rec_level){
		if (y == attributes){
			output(A, B);
			return;
		}
		queue<Rec> q;
		IntSet::Pool ints(attributes - y);
		ExtSet::Pool exts(attributes - y);
		ExtSet C;
		IntSet D;
		for (size_t j = y; j < attributes; j++) {
			if (!B.has(j)){
				C = exts.newEmpty();
				if (filterExtent(A, j, C)){ // if A == C
					B.add(j);
				}
				else{
					D = ints.newFull();
					partialClosure(C, j, D);
					if (B.equal(D, j)){ // equal up to <j
						q.emplace(C, D, j);
					}
					else
						stats.fail_canon++;
				}
			}
		}
		output(A, B);
		while (!q.empty()){
			Rec r = q.front();
			r.intent.copy(B);
			r.intent.add(r.j);
			if (rec_level == par_level)
				putToThread(r.extent, r.intent, r.j);
			else
				parInclose2Impl(r.extent, r.intent, r.j + 1, rec_level+1);
			q.pop();
		}
	}

	void inclose2(){
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet X = exts.newFull();
		IntSet Y = ints.newEmpty();
		inclose2Impl(X, Y, 0);
	}

	void parInclose2(){
		threads = new ThreadBlock[num_threads];
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet X = exts.newFull();
		IntSet Y = ints.newEmpty();
		parInclose2Impl(X, Y, 0, 0);
		// start of multi-threaded part
		vector<thread> trds(num_threads);

		for (size_t t = 0; t < num_threads; t++){
			trds[t] = thread([t, this]{
				size_t j;
				Context c = *this; // use separate context to count operations
				memset(&c.stats, 0, sizeof(c.stats));
				while (!threads[t].queue.empty()){
					threads[t].queue.fetch(threads[t].A, threads[t].B, j);
					c.inclose2Impl(threads[t].A, threads[t].B, j + 1);
				}
			});
		}
		for (auto & t : trds){
			t.join();
		}
	}

	void inclose3Impl(ExtSet A, IntSet B, size_t y, IntSet* N){
		if (y == attributes){
			output(A, B);
			return;
		}
		queue<Rec> q;
		IntSet::Pool ints(attributes - y);
		ExtSet::Pool exts(attributes - y);
		ExtSet C;
		IntSet D;
		IntSet* M = N + attributes;
		for (size_t j = y; j < attributes; j++) {
			M[j] = N[j];
			if (!B.has(j)){
				if (N[j].empty() || N[j].subsetOf(B, j)){ // subset of (considering attributes < j)
					C = exts.newEmpty();
					if (filterExtent(A, j, C)){ // if A == C
						B.add(j);
					}
					else{
						D = ints.newFull();
						partialClosure(C, j, D);
						if (B.equal(D, j)){ // equal up to <j
							q.emplace(C, D, j);
						}
						else {
							M[j] = D;
							stats.fail_canon++;
						}
					}
				}
				else
					stats.fail_fast++;
			}
		}
		output(A, B);
		while (!q.empty()){
			Rec r = q.front();
			r.intent.copy(B);
			r.intent.add(r.j);
			inclose3Impl(r.extent, r.intent, r.j + 1, M);
			q.pop();
		}
	}

	void parInclose3Impl(ExtSet A, IntSet B, size_t y, IntSet* N, size_t rec_level){
		if (y == attributes){
			output(A, B);
			return;
		}
		queue<Rec> q;
		IntSet::Pool ints(attributes - y);
		ExtSet::Pool exts(attributes - y);
		ExtSet C;
		IntSet D;
		IntSet* M = N + attributes;
		for (size_t j = y; j < attributes; j++) {
			M[j] = N[j];
			if (!B.has(j)){
				if (N[j].empty() || N[j].subsetOf(B, j)){ // subset of (considering attributes < j)
					C = exts.newEmpty();
					if (filterExtent(A, j, C)){ // if A == C
						B.add(j);
					}
					else{
						D = ints.newFull();
						partialClosure(C, j, D);
						if (B.equal(D, j)){ // equal up to <j
							q.emplace(C, D, j);
						}
						else {
							M[j] = D;
							stats.fail_canon++;
						}
					}
				}
				else
					stats.fail_fast++;
			}
		}
		output(A, B);
		while (!q.empty()){
			Rec r = q.front();
			r.intent.copy(B);
			r.intent.add(r.j);
			if (rec_level == par_level){
				memset(M, 0, sizeof(IntSet)* y); // clear first y IntSets that are possibly stale 
				putToThread(r.extent, r.intent, r.j, M);
			}
			else
				parInclose3Impl(r.extent, r.intent, r.j + 1, M, rec_level+1);
			q.pop();
		}
	}

	void parInclose3(){
		threads = new ThreadBlock[num_threads];
		for (size_t i = 0; i < num_threads; i++){
			threads[i].implied = new IntSet[max((size_t)2, attributes + 1 - par_level)*attributes];
			threads[i].M = IntSet::newArray(attributes);
		}
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet X = exts.newFull();
		IntSet Y = ints.newEmpty();
		// intents with implied error, see FCbO papper
		IntSet* implied = new IntSet[(par_level + 2)*attributes];
		parInclose3Impl(X, Y, 0, implied, 0);
		// start of multi-threaded part

		vector<thread> trds(num_threads);

		for (size_t t = 0; t < num_threads; t++){
			trds[t] = thread([t, this]{
				size_t j;
				Context c = *this; // use separate context to count operations
				memset(&c.stats, 0, sizeof(c.stats));
				while (!threads[t].queue.empty()){
					threads[t].queue.fetch(threads[t].A, threads[t].B, j, threads[t].M, attributes);
					for (size_t i = 0; i < attributes; i++){
						threads[t].implied[i] = threads[t].M[i];
					}
					c.inclose3Impl(threads[t].A, threads[t].B, j + 1, threads[t].implied);
				}
			});
		}
		for (auto & t : trds){
			t.join();
		}
	}

	void inclose3(){
		ExtSet::Pool exts(1);
		IntSet::Pool ints(1);
		ExtSet X = exts.newFull();
		IntSet Y = ints.newEmpty();
		// intents with implied error, see FCbO papper
		IntSet* implied = new IntSet[(attributes + 1)*attributes];
		inclose3Impl(X, Y, 0, implied);
	}

};

enum ALGO {
	NONE,
	CBO,
	FCBO, 
	INCLOSE2,
	INCLOSE3,
	PFCBO,
	PINCLOSE2,
	PINCLOSE3
};

ALGO fromName(const string& name){
	if (name == "cbo"){
		return CBO;
	}
	else if (name == "fcbo"){
		return FCBO;
	}
	else if (name == "inclose2"){
		return INCLOSE2;
	}
	else if (name == "inclose3"){
		return INCLOSE3;
	}
	else if (name == "pfcbo"){
		return PFCBO;
	}
	else if (name == "pinclose2"){
		return PINCLOSE2;
	}
	else if (name == "pinclose3"){
		return PINCLOSE3;
	}
	return NONE;
}

void start(Context& context, ALGO alg)
{
	switch (alg){
	case CBO:
		return context.cbo();
	case FCBO:
		return context.fcbo();
	case INCLOSE2:
		return context.inclose2();
	case INCLOSE3:
		return context.inclose3();
	case PFCBO:
		return context.parFcbo();
	case PINCLOSE2:
		return context.parInclose2();
	case PINCLOSE3:
		return context.parInclose3();
	}
}

int main(int argc, char* argv[])
{
	ios_base::sync_with_stdio(false);
	string arg;
	ALGO alg = NONE;
	ifstream in_file;
	ofstream out_file;
	size_t num_threads = 1;
	size_t par_level = 2;
	size_t verbose = 1;
	int i = 1;
	for (; i < argc && argv[i][0] == '-'; i++){
		switch (argv[i][1]){
		case 'a':
			// algorithm 
			arg = string(argv[i] + 2);
			alg = fromName(arg);
			if (alg == NONE){
				cerr << "No such algorithm " << arg << endl;
			}
			if (verbose == 2)
				cerr << "Using algorithm " << alg << endl;
			break;
		case 'v':
			verbose = atoi(argv[i] + 2);
			break;
		case 't':
			num_threads = atoi(argv[i] + 2);
			break;
		case 'L':
			par_level = atoi(argv[i] + 2);
			break;
		default:
			cerr << "Unrecognized option: " << argv[i] << endl;
		}
	}
	argv = argv + i;
	argc = argc - i;
	if (alg == NONE){
		cerr << "Algorithm not specified" << endl;
		return 1;
	}
	chrono::duration<double> elapsed;
	{
		Context context(verbose, num_threads, par_level);
		if (argc > 0){
			in_file.open(argv[0]);
			if (!context.loadFIMI(in_file)){
				cerr << "Failed to load any data.\n";
				return 1;
			}
		}
		else if (!context.loadFIMI(cin)){
			cerr << "Failed to load any data.\n";
			return 1;
		}
		if (argc > 1){
			out_file.open(argv[1]);
			context.setOutput(out_file);
		}
		if (verbose == 3){
			cerr << " Context:" << endl;
			context.printContext();
		}
		auto beg = chrono::high_resolution_clock::now();
		start(context, alg);
		auto end = chrono::high_resolution_clock::now();
		elapsed = end - beg;
	}
	cerr << "Time: " << elapsed.count() << endl;
	return 0;
}
