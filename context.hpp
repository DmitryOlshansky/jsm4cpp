#include "fimi.hpp"

using namespace std;
//TODO: add min_support filtering for InClose algorithms!
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
	size_t props_start; // where attributes end and properties start
	size_t min_support; // minimal support for hypotheses
	size_t* attributesNums; // sorted positions of attributes
	size_t* revMapping; // attributes to sorted positions
	ostream* out;
	//
	size_t verbose;
	size_t par_level;
	size_t num_threads;
	
	function<bool(IntSet)> filter;

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
				lock_guard<mutex> lock(output_mtx);
				string str = s.str();
				sink.write(str.c_str(), str.size());
			}
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
	Context(size_t verbose_, size_t threads_, size_t par_level_, size_t min_support_)
		:rows(), attributes(0), objects(0), min_support(min_support_), out(&cout), 
		verbose(verbose_), num_threads(threads_), par_level(par_level_){}
	~Context(){
		printStats();
	}

	void setOutput(ostream& sink){
		out = &sink;
	}

	void setFilter(function<bool (IntSet)> filter_){
		filter = filter_;
	}

	void toNaturalOrder(IntSet obj){
		IntSet::Pool slack(1);
		auto r = slack.newEmpty();
		obj.each([&](size_t i){
			r.add(attributesNums[i]);
		});
		obj.copy(r);
	}

	// print intent and/or extent
	void output(ExtSet A, IntSet B){
		if(verbose >= 1){
			if(!filter || filter(B))
				printAttributes(B, *out);
		}
	}

	void printContext(){
		for (size_t i = 0; i < objects; i++){
			printAttributes(rows[i]);
		}
	}

	bool loadFIMI(istream& inp, size_t total_attributes=0, size_t props=0){
		int max_attribute;
		vector<vector<int>> values = ::readFIMI(inp, &max_attribute);
		if (values.size() == 0){
			return false;
		}
		objects = values.size();
		if(total_attributes){
			attributes = total_attributes + props;
			props_start = total_attributes;
			if(attributes < max_attribute + 1){
				cerr << "Wrong total attributes override.\n";
				abort();
			}
		}
		else{
			attributes = max_attribute + 1;
			props_start = max_attribute + 1; //nowhere
		}
		
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
		
		revMapping = new size_t[attributes]; // from original to sorted (most frequent --> 0)
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
			if (rows[i].has(y)){
				C.add(i);
				D.intersect(rows[i]);
				cnt++;
			}
		});
		stats.closures++;
		return cnt >= min_support;
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
				if(closeConcept(A, j, C, D)){ // passed min support test
					if (B.equal(D, j)){ // equal up to <j
						cboImpl(C, D, j + 1);
					}
					else{
						stats.fail_canon++;
					}
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
					if(closeConcept(A, j, C, D) && B.equal(D, j)){ // equal up to <j
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
					if(closeConcept(A, j, C, D)){
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

