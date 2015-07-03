/**
	Platform module - time measurement and IO infrastructure.
*/
#pragma once


#include <chrono>
#include <cstdio>
#include <memory>
#include <mutex>
#include <ostream>

using namespace std;

//
template<class Fn>
void measure(Fn&& fn, const char* msg, bool condition=true){
	using namespace std::chrono;
	if(condition){
		duration<double> elapsed;
		auto beg = high_resolution_clock::now();
		fn();
		auto end = high_resolution_clock::now();
		elapsed = end - beg;
		cerr << msg << ": " << elapsed.count() << endl;
	}
	else
		fn();
}


// Simple I/O buffer with support for atomic portions of data (records).
// Only complete (committed) records would be ever written to the stream
class Buffer {
	char *buf;
	size_t size; // buffer size
	size_t cur; // current position
	size_t committed; // last committed position
	ostream* out_;
	shared_ptr<mutex> mut_;
	Buffer(const Buffer&)=delete;
public:
	Buffer(ostream& out, size_t sz): 
		 buf((char*)malloc(sz)), size(sz), cur(0), committed(0), out_(&out), 
		 mut_(make_shared<mutex>()){}
	//
	Buffer& sync(Buffer& b){
		mut_ = b.mut_;
	}
	// move over and null-ptr the buffer
	Buffer(Buffer&& b){
		buf = b.buf;
		size = b.size;
		cur = b.cur;
		committed = b.committed;
		out_ = b.out_;
		b.buf = nullptr;
	}
	// place c into buffer
	void put(char c){
		if(cur == size){
			if(committed > size * 63 / 64) // buffer should be large enough to hold ~64 records
				flush();
			else
				buf = (char*)realloc(buf, size*3/2);
		}
		buf[cur++] = c;
	}
	// place len bytes from data
	void put(char* data, size_t len){
		if(cur + len > size){
			if(committed > size * 63 / 64) // buffer should be large enough to hold ~64 records
				flush();
			else {
				while(cur + len > size)
					size = size * 3 / 2;
				buf = (char*)realloc(buf, size);
			}
		}
		memcpy(buf+cur, data, len);
		cur += len;
	}
	// change output stream
	Buffer& output(ostream& os){
		flush();
		out_ = &os;
		return *this;
	}
	// get current stream
	ostream& output(){
		return *out_;
	}
	// resets position to the last commited record
	Buffer& reset(){
		cur = committed;
		return *this;
	}
	// ends one "record" - atomic unit of data
	Buffer& commit(){
		committed = cur;
		return *this;
	}
	// flushes all commited data
	Buffer& flush(){
		if(committed){
			{
				lock_guard<mutex> lock(*mut_);
				out_->write(buf, committed);
			}
			// memmove - may overlap
			memmove(buf, buf+committed, size - committed);
			cur -= committed;
			committed = 0;
		}
		return *this;
	}
	//
	~Buffer(){
		if(buf){ //buf is empty after move
			flush();
			free(buf);
		}
	}
};

class TabledIntWriter {
	struct Entry{
		char len;
		char s[15];
	};
	vector<Entry> table;
public:
	TabledIntWriter(size_t max){
		table.resize(max);
		for(size_t i=0; i<max; i++){
			int cnt = sprintf(table[i].s, "%u", (unsigned)i);
			table[i].len = (char)cnt;
		}
	}

	void write(size_t val, Buffer& buf){
		auto& e = table[val];
		buf.put(e.s, e.len);
	}
};

class SimpleIntWriter {
public:
	SimpleIntWriter(size_t max){}
	void write(size_t val, Buffer& buf){
		char tmp[16];
		int cnt = sprintf(tmp, "%u", (unsigned) val);
		buf.put(tmp, cnt);
	}
};
