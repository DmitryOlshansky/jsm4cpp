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
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>

#include "fca.hpp"

using namespace std;

int main(int argc, char* argv[])
{
	ios_base::sync_with_stdio(false);
	string arg;
	unique_ptr<Algorithm> alg;
	ifstream in_file;
	ofstream out_file;
	size_t num_threads = 1;
	size_t par_level = 2;
	size_t verbose = 1;
	size_t min_support = 0;
	bool sort = false;
	int i = 1;
	for (; i < argc && argv[i][0] == '-'; i++){
		switch (argv[i][1]){
		case 'a':
			// algorithm 
			arg = string(argv[i] + 2);
			alg = fromName(arg);
			if (!alg){
				cerr << "No such algorithm " << arg << endl;
			}
			if (verbose == 2)
				cerr << "Using algorithm " << arg << endl;
			break;
		case 's':
			if(strcmp(argv[i], "-sort") != 0)
				goto L_unrecognized;
			sort = true;
			break;
		case 'm':
			// minimal support
			min_support = atoi(argv[i] + 2);
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
		L_unrecognized:
			cerr << "Unrecognized option: " << argv[i] << endl;
			exit(1);
		}
	}
	argv = argv + i;
	argc = argc - i;
	if (!alg){
		cerr << "Algorithm not specified" << endl;
		cerr << "Usage ./gen -a<algorithm> [-sort] [-v<verbosity>] [-L<par-level>] [-t<num-threads>]" << endl;
		return 1;
	}
	alg->verbose(verbose).threads(num_threads)
		.parLevel(par_level).minSupport(min_support);
	if (argc > 0){
		in_file.open(argv[0]);
		if (!alg->loadFIMI(in_file)){
			cerr << "Failed to load any data.\n";
			return 1;
		}
	}
	else if (!alg->loadFIMI(cin)){
		cerr << "Failed to load any data.\n";
		return 1;
	}
	if (argc > 1){
		out_file.open(argv[1]);
		alg->output(out_file);
	}
	measure([&]{ alg->run(); }, "Time: ", verbose > 0);
	return 0;
}
