#pragma once 

#include <algorithm>
#include <functional>
#include <mutex>
#include <memory>
#include <queue>

#include "sets.hpp"
#include "fimi.hpp"


using namespace std;
//TODO: add min_support filtering for InClose algorithms!
/**
	Algorithm is parametrized by 2 Set implementations.
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
			size = max(size * 3 / 2, needed);
			data = (char*)realloc(data, size);			
		}
	}
	char* data;
	size_t cur_w, size;
	size_t cur_r;
};


class Algorithm {
private:
	IntSet* rows; // attributes of objects
	size_t attributes_;
	size_t objects_;
	size_t props_start; // where attributes end and properties start
	size_t min_support_; // minimal support for hypotheses
	size_t* attributesNums; // sorted positions of attributes
	size_t* revMapping; // attributes to sorted positions
	ostream* out_;
	ostream* diag_;
	shared_ptr<mutex> output_mtx;
	//
	size_t verbose_;
	size_t par_level_;
	size_t threads_;
	
	function<bool(IntSet)> filter_;

	struct Stats{
		int total;
		int closures; // closures / partial closures computed
		int fail_canon; // canonical test failures
		int fail_fast; // fast canonical test failures
		Stats(): total(0), closures(0), fail_canon(0), fail_fast(0){}
	};

	
	void printAttributes(IntSet& set, ostream& sink){
		if (verbose() >= 1){
			stringstream s;
			bool nonempty = false;
			set.each([&](size_t i){
				size_t attr = attributesNums[i];
				if(attr < props_start)
					nonempty = true;
				s << attr << ' ';
			});
			s << '\n';
			if(nonempty)
			{
				lock_guard<mutex> lock(*output_mtx);
				string str = s.str();
				sink.write(str.c_str(), str.size());
			}
		}
		stats.total++;
	}

	void printStats(){
		if (verbose() >= 2){
			lock_guard<mutex> lock(*output_mtx);
			*diag_ << "Total\tClosure\tCanonical\tFast\n"
			     << stats.total << '\t'<< stats.closures 
				 << '\t' << stats.fail_canon << '\t' << stats.fail_fast << endl;
		}
	}

	virtual void algorithm()=0;
protected:
	
	// print intent and/or extent
	void output(ExtSet A, IntSet B){
		if(verbose() >= 1){
			if(!filter_ || filter_(B))
				printAttributes(B, *out_);
		}
	}
public:
	Stats stats; // TODO: hackish

	Algorithm():rows(), attributes_(0), objects_(0), min_support_(0), out_(&cout), 
	diag_(&cerr), verbose_(false), threads_(0), par_level_(0){
		output_mtx = make_shared<mutex>();
	}

	// clone & reuse most of current algorithm's state but with empty stats
	template<class Algo>
	Algo fork()
	{
		Algo algo;
		algo.rows = rows;
		algo.attributes_ = attributes_;
		algo.objects_ = objects_;
		algo.min_support_ = min_support_;
		algo.out_ =  out_;
		algo.diag_ = diag_;
		algo.verbose_ = verbose_;
		algo.threads_ = threads_;
		algo.par_level_ = par_level_;
		algo.props_start = props_start;
		algo.attributesNums = attributesNums;
		algo.revMapping = revMapping;
		algo.output_mtx = output_mtx;
		return algo;
	}

	virtual ~Algorithm(){
		printStats();
	}

	// Get/set verbose level
	size_t verbose()const { return verbose_; }
	Algorithm& verbose(bool verboseVal){ 
		verbose_ = verboseVal;
		return *this;
	}

	// Get/set numbers of threads in the pool
	size_t threads()const{ return threads_; }
	Algorithm& threads(size_t thrds){
		threads_ = thrds;
		return *this;
	}

	// Get/set minimal required support for hypothesis/concept
	size_t minSupport()const{ return min_support_; }
	Algorithm& minSupport(size_t min_sup){
		min_support_ = min_sup;
		return *this;
	}

	// Get/set max serial recursion depth
	size_t parLevel()const{ return par_level_; }
	Algorithm& parLevel(size_t par_lvl){
		par_level_ = par_lvl;
		return *this;
	}

	// Get/set ostream for output
	Algorithm& output(ostream& sink){
		out_ = &sink;
		return *this;
	}

	// Get/set ostream for diagnostics
	Algorithm& diagnostic(ostream& sink){
		diag_ = &sink;
		return *this;
	}

	// Get/set function to filter out set of attributes as proper hypothesis
	Algorithm& filter(function<bool (IntSet)> filt){
		filter_ = filt;
		return *this;
	}
	// Get dimensions of loaded Algorithm
	size_t attributes()const{ return attributes_; }
	size_t objects()const{ return objects_; }

	//
	IntSet& row(size_t i){ return rows[i]; }

	void toNaturalOrder(IntSet obj){
		IntSet::Pool slack(1);
		auto r = slack.newEmpty();
		obj.each([&](size_t i){
			r.add(attributesNums[i]);
		});
		obj.copy(r);
	}

	// Run specified algorithm with current parameters and data
	void run(){
		algorithm();
	}

	void printContext(){
		for (size_t i = 0; i < objects(); i++){
			printAttributes(row(i), *diag_);
		}
	}

	bool loadFIMI(istream& inp, size_t total_attributes=0, size_t props=0){
		int max_attribute;
		vector<vector<int>> values = ::readFIMI(inp, &max_attribute);
		if (values.size() == 0){
			return false;
		}
		objects_ = values.size();
		if(total_attributes){
			attributes_ = total_attributes + props;
			props_start = total_attributes;
			if(attributes_ < max_attribute + 1){
				*diag_ << "Wrong total attributes override.\n";
				abort();
			}
		}
		else{
			attributes_ = max_attribute + 1;
			props_start = max_attribute + 1; //nowhere
		}
		
		// Count supports and sort Algorithm
		// May also cut off attributes based on minimal support here and resize accordingly
		vector<size_t> supps(attributes_);
		fill(supps.begin(), supps.end(), 0);

		attributesNums = new size_t[attributes_];
		for (size_t i = 0; i < attributes_; i++){
			attributesNums[i] = i;
		}

		for (size_t i = 0; i < objects_; i++){
			for (auto val : values[i]){
				supps[val]++;
			}
		}

		// attributeNums[0] --> least frequent attribute num
		sort(attributesNums, attributesNums+attributes_, [&](size_t i, size_t j){
			return supps[i] < supps[j];
		});
		
		revMapping = new size_t[attributes_]; // from original to sorted (most frequent --> 0)
		for (size_t i = 0; i < attributes_; i++){
			revMapping[attributesNums[i]] = i;
		}

		// Setup Algorithm with re-ordered attributes
		ExtSet::setSize(objects_);
		IntSet::setSize(attributes_);
		rows = IntSet::newArray(objects_);
		for (size_t i = 0; i < objects_; i++){
			row(i).clearAll();
			for (auto val : values[i]){
				row(i).add(revMapping[val]);
			}
		}
		return true;
	}

	// get sorted mapping of attribute n
	size_t mapAttribute(size_t n){
		return revMapping[n];
	}

	// load and identically reorder attributes
	bool readFIMI(istream& inp, IntSet** sets, size_t* size)
	{
		int max_attribute;
		vector<vector<int>> values = ::readFIMI(inp, &max_attribute);
		if(values.size() == 0){
			*size = 0;
			return false;
		}
		auto objs = IntSet::newArray(values.size());
		for (size_t i = 0; i < values.size(); i++){
			objs[i].clearAll();
			for (auto val : values[i]){
				objs[i].add(revMapping[val]);
			}
		}
		*size = values.size();
		*sets = objs;
		return true;
	}

	/**
		Inputs: A - extent, B - intent, y - new attribute
		C - empty, D - full
		Output: C = A intersect closure({j}); D = closure of C
		Returns: boolean - true if extent passes the minimal support
	*/
	bool closeConcept(ExtSet A, size_t y, ExtSet C, IntSet D){
		size_t cnt = 0;
		A.each([&](size_t i){
			if (row(i).has(y)){
				C.add(i);
				D.intersect(row(i));
				cnt++;
			}
		});
		stats.closures++;
		return cnt >= minSupport();
	}

	// Produce extent having attribute y from A
	// true - if produced extent is identical
	bool filterExtent(ExtSet A, size_t y, ExtSet C){
		bool ret = true;
		//C.clearAll();
		A.each([&](size_t i){
			if (row(i).has(y)){
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
			D.intersect(row(i), y);
		});
		stats.closures++;
	}

};

// Algorithms that use queue to do recursion layer after layer
class HybridAlgorithm : virtual public Algorithm {
public:
	using Algorithm::Algorithm;

	struct Rec{ // internal struct for enqueing (F)CbO calls 
		ExtSet extent;
		IntSet intent;
		size_t j;
		Rec(ExtSet e, IntSet i, size_t y) :extent(e), intent(i), j(y){}
	};
};

// Algorithms that use thread pool to run many sub-tasks in parallel
template<class State, class BaseAlgo>
class ParallelAlgorithm : virtual public Algorithm{
	enum { QUEUE_SIZE = 16 * 1024 };
	struct ThreadBlock{
		BlockQueue queue;
		IntSet* implied; // if has fast test, stack of (attributes - par_level)*attributes pointers
		IntSet* M; // if has fast test, real flat array of intents used to unpack from queue
		ThreadBlock() :queue(QUEUE_SIZE){}
	};

	vector<ThreadBlock> threadBlks;
	virtual void serialStep()=0; // main serial algorithm - process up to level L

	// generic parallel algorithm using serialStep and base algorithm for each sub-task
	void algorithm(){
		threadBlks.resize(threads());
		serialStep();
		// start of multi-threaded part
		vector<thread> trds(threads());

		for (size_t t = 0; t < threads(); t++){
			trds[t] = thread([this, t]{
				State state;
				ExtSet::Pool exts(1);
				IntSet::Pool ints(1);
				state.extent = exts.newEmpty();
				state.intent = ints.newEmpty();
				state.alloc(*this);
				auto sub = fork<BaseAlgo>();
				while (extract(t, state)){
					sub.run(state);
				}
			});
		}
		for (auto & t : trds){
			t.join();
		}
	}
protected:	
	void schedule(State state){
		static int tid = 0;
		state.save(threadBlks[tid].queue);
		tid += 1;
		if (tid == threads())
			tid = 0;
	}

 	// get next item for the worker thread 'tid'
	bool extract(size_t tid, State& state){
		if(threadBlks[tid].queue.empty()){
			return false;
		}
		state.load(threadBlks[tid].queue);
		return true;
	}
};


// Minimalistic state for InClose2/CbO call
struct SimpleState {
	ExtSet extent;
	IntSet intent;
	size_t j; // attribute #

	void alloc(Algorithm& algo){}

	void save(BlockQueue& queue){
		queue.put(extent, intent, j);
	}

	void load(BlockQueue& queue){
		queue.fetch(extent, intent, j);
	}
};

// Extended state for algorithms with implied errors array
struct ExtendedState {
	ExtSet extent;
	IntSet intent;
	size_t j; // attribute #
	IntSet* implied; // must be inited
	size_t attributes;

	void alloc(Algorithm& algo){
		implied = new IntSet[max((size_t)2, algo.attributes() + 1 - algo.parLevel())*algo.attributes()];
	}

	void save(BlockQueue& queue){
		queue.put(extent, intent, j);
	}

	void load(BlockQueue& queue){
		queue.fetch(extent, intent, j);
	}

	~ExtendedState(){
		delete[] implied;
	}
};
