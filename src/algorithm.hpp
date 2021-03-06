#pragma once 

#include <algorithm>
#include <functional>
#include <memory>
#include <queue>

#include "fimi.hpp"
#include "platform.hpp"
#include "queues.hpp"
#include "sets.hpp"

using namespace std;
//TODO: add min_support filtering for InClose algorithms!
/**
	Algorithm is parametrized by:
		- 2 Set implementations (Extent/Intent)
		- Writer implementation
	At a deeper level sets are parametrized by allocator type.
	Set objects typically amount to >97% of all allocations.
*/
#if defined(USE_LINEAR_EXT)
	using ExtSet = LinearSet;
#elif defined(USE_BIT_EXT)
	using ExtSet = BitVec<0>;
#elif defined(USE_TREE_EXT)
	using ExtSet = TreeSet;
#else
	#error "Must use some implementation for Extent!"
#endif

#if defined(USE_LINEAR_INT)
	using IntSet = LinearSet;
#elif defined(USE_BIT_INT)
	using IntSet = BitVec<1>;
#elif defined(USE_TREE_INT)
	using IntSet = TreeSet;
#else
	#error "Must use some implementation for Intent!"
#endif

#if defined(USE_TABLE_WRITER)
	using IntWriter = TabledIntWriter;
#elif  defined(USE_SIMPLE_WRITER)
	using IntWriter = SimpleIntWriter;
#else
	#error "Must define one of legal USE_xxx_WRITER"
#endif

using CompIntSet = CompressedSet<IntSet>;

class Algorithm {
private:
	IntSet* rows; // attributes of objects
	size_t attributes_;
	size_t objects_;
	size_t props_start; // where attributes end and properties start
	size_t min_support_; // minimal support for hypotheses
	size_t* attributesNums; // sorted positions of attributes
	size_t* revMapping; // attributes to sorted positions
	// ostream* out_;
	ostream* diag_;
	Buffer buf;
	shared_ptr<mutex> output_mtx;
	shared_ptr<IntWriter> writer;
	//
	bool sort_;
	size_t verbose_;
	size_t par_level_;
	size_t threads_;
	
	function<bool(IntSet&)> filter_;

	struct Stats{
		int total;
		int closures; // closures / partial closures computed
		int fail_canon; // canonical test failures
		int fail_fast; // fast canonical test failures
		Stats(): total(0), closures(0), fail_canon(0), fail_fast(0){}
	};
protected:
	
	void printAttributes(IntSet& set){
		if (verbose() >= 1){
			bool nonempty = false;
			bool need_ws = false;
			set.each([&](size_t i){
				size_t attr = attributesNums[i];
				if(need_ws)
					buf.put(' ');
				else
					need_ws = true;
				if(attr < props_start)
					nonempty = true;
				writer->write(attr, buf);
			});
			buf.put('\n');
			if(nonempty){
				buf.commit();
			}
			else
				buf.reset();
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

protected:
	virtual void algorithm()=0;

	// print intent and/or extent
	virtual void output(ExtSet& A, IntSet& B){
		if(verbose() >= 1){
			if(!filter_ || filter_(B))
				printAttributes(B);
		}
	}
public:
	Stats stats; // TODO: hackish

	Algorithm():rows(), attributes_(0), objects_(0), min_support_(0),
		output_mtx(make_shared<mutex>()), 
		buf(cout), diag_(&cerr), sort_(false),
		verbose_(0), threads_(0), par_level_(0){}

	Algorithm(Algorithm&& algo):
		rows(move(algo.rows)), 
		attributes_(algo.attributes_), objects_(algo.objects_), 
		min_support_(algo.min_support_), output_mtx(algo.output_mtx),
		diag_(algo.diag_), verbose_(algo.verbose_), 
		threads_(algo.threads_), par_level_(algo.par_level_), 
		buf(move(algo.buf)), stats(algo.stats){}
	// clone & reuse most of current algorithm's state but with empty stats
	template<class Algo>
	Algo fork()
	{
		Algo algo;
		algo.rows = rows;
		algo.attributes_ = attributes_;
		algo.objects_ = objects_;
		algo.min_support_ = min_support_;
		algo.output(buf.output());
		algo.diag_ = diag_;
		algo.sort_ = sort_;
		algo.verbose_ = verbose_;
		algo.threads_ = threads_;
		algo.par_level_ = par_level_;
		algo.props_start = props_start;
		algo.attributesNums = attributesNums;
		algo.revMapping = revMapping;
		algo.buf.sync(buf);
		algo.writer = writer;
		return algo;
	}

	virtual ~Algorithm(){
		printStats();
	}

	// Get/set sort option
	bool sortAttrs()const{ return sort_; }
	Algorithm& sortAttrs(bool s){ 
		sort_ = s;
		return *this;
	}

	// Get/set verbose level
	size_t verbose()const { return verbose_; }
	Algorithm& verbose(size_t verboseVal){ 
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
	// Get/set IO buffer size
	Algorithm& bufferSize(size_t sz){
		buf.resize(sz);
		return *this;
	}
	size_t bufferSize()const{
		return buf.size();
	}

	// Get/set ostream for output
	Algorithm& output(ostream& sink){
		buf.output(sink);
		return *this;
	}

	// Get/set ostream for diagnostics
	Algorithm& diagnostic(ostream& sink){
		diag_ = &sink;
		return *this;
	}

	// Get/set function to filter out set of attributes as proper hypothesis
	Algorithm& filter(function<bool (IntSet&)> filt){
		filter_ = filt;
		return *this;
	}
	// Get dimensions of loaded Algorithm
	size_t attributes()const{ return attributes_; }
	size_t objects()const{ return objects_; }

	//
	IntSet& row(size_t i){ return rows[i]; }

	void toNaturalOrder(IntSet& obj){
		IntSet r = IntSet::newEmpty();
		obj.each([&](size_t i){
			r.add(attributesNums[i]);
		});
		obj.copy(r);
	}

	// Run specified algorithm with current parameters and data
	void run(){
		algorithm();
		lock_guard<mutex> lock(*output_mtx);
		buf.flush();
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
		if(verbose() > 1){
			size_t ones = 0;
			for(size_t i=0; i<attributes_; i++)
				ones += supps[i];
			auto density = ones / (double)(attributes_*objects_);
			cerr << "Density of ones: " << density << endl;
		}
		// attributeNums[0] --> least frequent attribute num
		if(sort_){
			sort(attributesNums, attributesNums+attributes_, [&](size_t i, size_t j){
				return supps[i] < supps[j];
			});
		}
		
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
		writer = make_shared<IntWriter>(attributes());
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
		Inputs: A - extent, y - new attribute
		C - empty, D - full
		Output: C = A intersect closure({j}); D = closure of C
		Returns: boolean - true if extent passes the minimal support
	*/
	bool closeConcept(ExtSet& A, size_t y, ExtSet& C, IntSet& D){
		size_t cnt = 0;
		//cerr << "y = " << y << endl;
		A.each([&](size_t i){
			if (row(i).has(y)){
				/*cerr << "+++ ";
				printSet(row(i),cerr);
				cerr << endl;*/
				C.add(i);
				D.intersect(row(i));

				cnt++;
			}
		});
		/*cerr << "=== ";
		printSet(D,cerr);
		cerr << endl;*/
		stats.closures++;
		return cnt >= minSupport();
	}

	// Produce extent having attribute y from A
	// true - if produced extent is identical
	bool filterExtent(ExtSet& A, size_t y, ExtSet& C){
		bool ret = true;
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
	void partialClosure(ExtSet& C, size_t y, IntSet& D){
		C.each([&](size_t i){
			D.intersect(row(i), y);
		});
		stats.closures++;
	}

};

// Algorithms that use queue to do recursion layer after layer
class HybridAlgorithm : virtual public Algorithm{
public:
	using Algorithm::Algorithm;

	struct Rec{ // internal struct for enqueing (F)CbO calls 
		ExtSet extent;
		IntSet intent;
		size_t j;
		Rec(ExtSet e, IntSet i, size_t y) :extent(move(e)), intent(move(i)), j(y){}
	};

};


// Minimalistic state for InClose2/CbO call
struct SimpleState {
	ExtSet extent;
	IntSet intent;
	size_t j; // attribute #

	void alloc(Algorithm& algo){}
	SimpleState& dup(){ return *this; } //nothing to duplicate
};

// Extended state for algorithms with implied errors array
struct ExtendedState {
	ExtSet extent;
	IntSet intent;
	size_t j; // attribute #
	CompIntSet* implied; // must be inited
	size_t attributes;
	bool owns;
	ExtendedState():extent(), intent(), j(0), implied(nullptr), attributes(0), owns(false){}
	ExtendedState(ExtSet extent_, IntSet intent_, size_t attr, CompIntSet* implied_, size_t total_attributes):
		extent(move(extent_)), intent(move(intent_)), j(attr), implied(implied_), attributes(total_attributes), owns(false){}

	ExtendedState(ExtendedState&& state):implied(nullptr)
	{
		*this = move(state);
	}

	ExtendedState& operator=(ExtendedState&& state)
	{
		extent = move(state.extent);
		intent = move(state.intent);
		j = state.j;
		attributes = state.attributes;
		if(implied){ // assume we have preallocated implied stack that is X*attributes in size
			for(size_t i=j; i<state.attributes; i++){ // no point in copying sets < j, recursion goes from j
				implied[i] = state.implied[i];
			}
			owns = state.owns;
			state.implied = nullptr;
		}
		else{
			implied = state.implied;
			owns = state.owns;
			state.implied = nullptr;
		}
		return *this;
	}

	// allocate new stack for implied errors 
	// (normally state just refrences one layer of common stack)
	void alloc(Algorithm& algo){
		implied = new CompIntSet[max((size_t)2, algo.attributes() + 1 - algo.parLevel())*algo.attributes()];
		owns = true;
	}

	// duplicate implied vector
	ExtendedState& dup(){
		auto new_implied = new CompIntSet[attributes];
		copy(implied, implied+attributes, new_implied);
		implied = new_implied;
		owns = true;
		return *this;
	}

	~ExtendedState(){
		if(owns) delete[] implied; // if we allocted it delete implied vector
	}
};


// plain strategy that doesn't schedule threads
class RecursiveStrategy{
protected:
	template<class Algo>
	void processQueueItem(Algo* _this, typename Algo::State& state){
		_this->run(state); // assume correct mixing
	}
};

class SchedulingCutoffStrategy{
	size_t rec_depth;
protected:	
	SchedulingCutoffStrategy():rec_depth(0){}
	template<class Algo>
	void processQueueItem(Algo* _this, typename Algo::State& state){
		if(rec_depth == _this->parLevel())
			_this->schedule(move(state.dup())); // reallocte implied vector
		else{
			rec_depth++;
			_this->run(state);
			rec_depth--;
		}
	}
};

// Algorithms that use single serial step and follow through it with recursive calls
template<class GenericAlgo>
class RecursiveCalls: public GenericAlgo, public RecursiveStrategy {
public:
	using State = typename GenericAlgo::State;
	void processQueueItem(State&& s){
		RecursiveStrategy::processQueueItem(this, s);
	}
};

// Algorithms that use serial step to generate tasks, followed by a fork-join of parallel execution
// Parallel execution may use a different algorithm then the serial one, as long as they agree on
// state represenation (Algo::State)
template<class GenericAlgo, class SerialAlgo>
class ForkJoin : public GenericAlgo, virtual public Algorithm, public SchedulingCutoffStrategy {	
public:
	using State = typename GenericAlgo::State;
	using GenericAlgo::GenericAlgo;
private:
	vector<queue<State> > queues;

	// generic parallel algorithm using serialStep and base algorithm for each sub-task
	void algorithm(){
		queues = vector<queue<State>>(threads());
		measure([&]{
			serial(); //serial step
		}, "Serial", verbose() > 1);

		// start of multi-threaded part
		vector<thread> trds(threads());
		measure([&]{
			for (size_t t = 0; t < threads(); t++){
				trds[t] = thread([this, t]{
					State state;
					state.extent = ExtSet::newEmpty();
					state.intent = IntSet::newEmpty();
					state.alloc(*this);
					auto sub = fork<SerialAlgo>();
					while (extract(t, state)){
						sub.run(state);
					}
					//state.dispose();
				});
			}
		}, "Starting threads", verbose() > 1);
		for (auto & t : trds){
			t.join();
		}
	}

 	// get next item for the worker thread 'tid'
	bool extract(size_t tid, State& state){
		auto &q = queues[tid];
		if(q.empty()){
			return false;
		}
		state = move(q.front());
		q.pop();
		return true;
	}
	
	void serial(){
		GenericAlgo::algorithm();
	}

	void processQueueItem(State&& s){
		SchedulingCutoffStrategy::processQueueItem(this, s);
	}
public:
	void schedule(State state){
		static int tid = 0;
		queues[tid].push(move(state));
		tid += 1;
		if (tid == threads())
			tid = 0;
	}

};


// Same as fork-join execution but using shared queue to assert fair distribution of work load
template<class GenericAlgo, class SerialAlgo>
class FairForkJoin: public GenericAlgo, virtual public Algorithm, public SchedulingCutoffStrategy {	
public:
	using State = typename GenericAlgo::State;
	using GenericAlgo::GenericAlgo;
private:
	SyncronizedQueue<State> queue;

	// generic parallel algorithm using serialStep and base algorithm for each sub-task
	void algorithm(){
		measure([&]{
			serial();
		}, "Serial step", verbose() > 1);
		// start of multi-threaded part
		if(threads()){
			size_t tpool_size = threads()-1;
			vector<thread> trds(tpool_size);
			measure([&]{
				for (size_t t = 0; t < tpool_size; t++){
					trds[t] = thread([this]{ workThread();	});
				}
			}, "Starting threads", verbose() > 1);
			workThread();
			for (auto & t : trds){
				t.join();
			}
		}
		else
			workThread();
	}

	void processQueueItem(State&& s){
		SchedulingCutoffStrategy::processQueueItem(this, s);
	}

	void workThread(){
		State state;
		state.extent = ExtSet::newEmpty();
		state.intent = IntSet::newEmpty();
		state.alloc(*this);
		auto sub = fork<SerialAlgo>();
		while (queue.pop(state)){
			sub.run(state);
		}
		//state.dispose();
	}
	void serial(){
		GenericAlgo::algorithm();
	}
public:
	void schedule(State&& state){
		queue.push(move(state));
	}

};


template<class GenericAlgo, class SerialAlgo>
class WithThreadPool: public GenericAlgo, virtual public Algorithm, public SchedulingCutoffStrategy {	
public:
	using State = typename GenericAlgo::State;
	using GenericAlgo::GenericAlgo;
private:
	SharedQueue<State> queue;

	void algorithm(){
		if(threads()){
			size_t tpool_size = threads()-1;
			vector<thread> trds(tpool_size);
			measure([&]{
				for (size_t t = 0; t < tpool_size; t++){
					trds[t] = thread([this]{ workThread(); });
				}
			}, "Starting threads", verbose() > 1);
			mainThread();
			for (auto & t : trds){ // join pooled threads
				t.join();
			}
		}
		else
			mainThread();
	}

	// combines scheduling step and parallel worker step
	void mainThread(){
		measure([&]{
			serial();
		}, "Serial step", verbose() > 1);
		workThread();
	}

	void workThread(){
		State state;
		state.extent = ExtSet::newEmpty();
		state.intent = IntSet::newEmpty();
		state.alloc(*this);
		auto sub = fork<SerialAlgo>();
		while (queue.pop(state)){
				sub.run(state);
		}
		//state.dispose();
	}

	void serial(){
		GenericAlgo::algorithm(); //serial step
		queue.close();
	}

	void processQueueItem(State&& s){
		SchedulingCutoffStrategy::processQueueItem(this, s);
	}
public:
	void schedule(State&& state){
		queue.push(move(state));
	}

};

template<class GenericAlgo>
class WaveFrontSingle : public GenericAlgo {
	using State = typename GenericAlgo::State;
	size_t rank_, waveSize_;
	size_t counter_, rec_depth_;
	void processQueueItem(State&& s){
		if(rec_depth_  == this->parLevel())
		{
			if(counter_ == rank_)
			{
				rec_depth_++;
				GenericAlgo::run(s);
				rec_depth_--;
			}
			counter_++;
			if(counter_ == waveSize_)
				counter_ = 0;
		}
		else
		{
			rec_depth_++;
			GenericAlgo::run(s);
			rec_depth_--;
		}
	}
protected:
	void output(ExtSet& A, IntSet& B){
		if(rank_ == 0 || rec_depth_ >= this->parLevel())
			Algorithm::output(A, B);
	}
public:
	void output(ostream& os){ Algorithm::output(os); }
	void run(){ Algorithm::run(); }
	WaveFrontSingle():
		Algorithm(),rank_(0),waveSize_(0), counter_(0),rec_depth_(0){}
	//
	WaveFrontSingle& rank(size_t r){ rank_  = r; return *this; }
	//
	WaveFrontSingle& waveSize(size_t ws){ waveSize_  = ws; return *this; }
};

template<class GenericAlgo>
class WaveFrontParallel : public Algorithm {
	void algorithm(){
		vector<thread> thrds;
		size_t total = threads();
		auto this_ = this;
		for(size_t i =0; i<total; i++){
			thrds.emplace_back([i, total, this_]{
				auto algo = this_->fork<WaveFrontSingle<GenericAlgo>>();
				algo.rank(i);
				algo.waveSize(total);
				algo.run();
			});
		}
		for(auto& t : thrds)
			t.join();
	}
public:
	using Algorithm::Algorithm;
};

