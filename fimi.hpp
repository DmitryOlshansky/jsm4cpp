#pragma once 
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

inline std::vector<std::vector<int>> readFIMI(std::istream& inp, int* max_attribute){
	vector<vector<int>> values;
	int max_attr = -1;
	string s;
	while (getline(inp, s)){
		vector<int> vals;
		stringstream ss(s);
		for (;;){
			int val = -1;
			ss >> val;
			if (val == -1)
				break;
			if (max_attr < val){
				max_attr = val;
			}
			vals.push_back(val);
		}
		values.push_back(move(vals));
	}
	if(max_attribute)
		*max_attribute = max_attr;
	return values;
}