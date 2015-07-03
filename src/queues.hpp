#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

using namespace std;

// Simple mutex-based synchronized queue
template<class T>
class SyncronizedQueue{
	queue<T> queue_;
	mutex mtx_;
public:
	bool pop(T& val){
		lock_guard<mutex> lock(mtx_);
		if(queue_.empty())
			return false;
		val = move(queue_.front());
		queue_.pop();
		return true;
	}

	void push(T&& val){
		lock_guard<mutex> lock(mtx_);
		queue_.push(move(val));
	}
};

// Shared  queue - supports simlutanious pushing and popping.
// May be drained multiple times in the process,
// closing the queue is thus a separate primitive
template<class T>
class SharedQueue{
	queue<T> queue_;
	mutex mtx_;
	condition_variable cond_;
	bool done_;
public:
	SharedQueue():done_(false){}
	bool pop(T& val){
		unique_lock<mutex> lock(mtx_);
		cond_.wait(lock, [&]{ return !queue_.empty() || done_; });
		if(queue_.empty()) // done -> and queue is empty
			return false;
		val = move(queue_.front());
		queue_.pop();
		return true;
	}

	void push(T&& val){
		unique_lock<mutex> lock(mtx_);
		assert(!done_); // must not push after closing the queue
		queue_.push(move(val));
		lock.unlock();
		cond_.notify_one();
	}

	// "close" the queue - no more new elements are going to be pushed
	void close(){
		unique_lock<mutex> lock(mtx_);
		done_ = true;
		cond_.notify_all();
	}
};