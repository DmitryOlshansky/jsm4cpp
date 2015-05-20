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
#include <cstdlib>
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

#include "sets.hpp"
#include "context.hpp"

using namespace std;

void usage(){
	cerr << "Usage ./jsm -a<algorithm> -m<min-support> -s<attributes> -p<props> "
		"-i+<plus-file> -i-<minus-file> -o<hyp-file> [-v<verbosity>] [-L<par-level>]"
		"[-t<num-threads>]\n";
	exit(1);
}

int main(int argc, char* argv[])
{
	ios_base::sync_with_stdio(false);
	string arg;
	ALGO alg = NONE;
	string plus_in, minus_in;
	string hyp_out;
	size_t num_threads = 1;
	size_t par_level = 2;
	size_t verbose = 1;
	size_t min_support = 2;
	size_t attributes = 0;
	size_t props = 0;
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
		case 's':
			attributes = atoi(argv[i] + 2);
			break;
		case 'p':
			props = atoi(argv[i] + 2);
			break;
		case 'm':
			// minimal support
			min_support = atoi(argv[i] + 2);
			break;
		case 'i':
			if(argv[i][2] == '+'){
				plus_in = string(argv[i] + 3);
			}
			else if(argv[i][2] == '-'){
				minus_in = string(argv[i] + 3);
			}
			break;
		case 'o':
			hyp_out = string(argv[i] + 2);
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
	if(attributes == 0){
		cerr << "No attributes specified" << endl;
		usage();
	}
	if(props == 0){
		cerr << "No properties specified" << endl;
		usage();
	}
	if (alg == NONE){
		cerr << "Algorithm not specified" << endl;
		usage();
	}
	if (plus_in.empty()){
		cerr << "No plus input file" << endl;
		usage();
	}
	if (minus_in.empty()){
		cerr << "No minus input file" << endl;
		usage();
	}
	if (hyp_out.empty()){
		cerr << "No hypotheses output file" << endl;
		usage();
	}
	chrono::duration<double> elapsed;
	{
		Context context(verbose, num_threads, par_level, min_support);
		ifstream plus_stream(plus_in.c_str());
		ifstream minus_stream(minus_in.c_str());
		ofstream hyp_stream(hyp_out.c_str());

		if(!context.loadFIMI(plus_stream, attributes, props)){
			cerr << "Failed to load any examples" << endl;
			exit(1);
		}
		IntSet* minus;
		size_t minus_size;
		context.readFIMI(minus_stream, &minus, &minus_size);
		cerr << "Minus examples:" << minus_size << endl;

		context.setOutput(hyp_stream);

		auto beg = chrono::high_resolution_clock::now();
		start(context, alg);
		auto end = chrono::high_resolution_clock::now();
		elapsed = end - beg;
	}
	cerr << "Time: " << elapsed.count() << endl;
	return 0;
}
