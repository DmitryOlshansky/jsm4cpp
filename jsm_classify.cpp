/**
	Классификация примеров для Химии.
	Свойства:
	10 - плюс
	01 - минус
	11 - тау
	00 - противоречие
*/
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

#include "fimi.hpp"
#include "sets.hpp"

template<>
size_t BitVec<0>::words = 0;
template<>
size_t BitVec<0>::length = 0;

using Set = BitVec<0>;

void usage(){
	cerr << "Usage ./jsm_classify -s<attributes> -p<props> "
		"-+<plus-file> --<minus-file> -t<tau-file> -o<output-json-file>\n";
	exit(1);
}

Set toSet(const vector<int>& vec, Set::Pool& pool){
	Set ret = pool.newEmpty();
	for_each(vec.begin(), vec.end(), [&ret](int a){
		ret.add(a);
	});
	return ret;
}

vector<Set> toSets(const vector<vector<int>>& vec, Set::Pool& pool){
	vector<Set> ret(vec.size());
	for(size_t i=0; i<vec.size(); i++)
		ret[i] = toSet(vec[i], pool);
	return ret;
}

vector<size_t> allIncludes(Set example, vector<Set>& set){
	vector<size_t> found;
	Set::Pool temp(1);
	Set val = temp.newEmpty();
	for(size_t i=0; i<set.size(); i++){
		val.copy(example);
		val.intersect(set[i]);
		if(val.equal(set[i])){ //вложение (тау имеет '1' во всех свойствах)
			found.push_back(i);
		}
	}
	return found;
}

void intersectAll(vector<Set>& which, const vector<size_t> idx, Set dest)
{
	for(auto i : idx){
		dest.intersect(which[i]);
	}
}

void write_array(const vector<size_t>& nums, ostream& output){
	output << "[";
	for(size_t i=0; i<nums.size(); i++){
		if(i != 0){
			output << ", ";
		}
		output << nums[i];
	}
	output<< "]";
}

// Вывести свойства как (+)*(?)*(-)* строку
void write_activity(Set set, size_t attrs, size_t props, ostream& output){
	// свойства идут парами
	for(size_t i=attrs; i<attrs+props; i+=2){
		bool a1 = set.has(i), a2 = set.has(i+1);
		if(a1 && a2)
			output << '?';
		else if(a1)
			output << '+';
		else if(a2)
			output << '-';
		else
			output << '0';
	}
}

// 
void classify(size_t attrs, size_t props, vector<Set>& plus, 
vector<Set>& minus, vector<Set>& tau, ostream& output){
	output << "[\n";
	for(size_t i=0; i<tau.size(); i++){
		auto& t = tau[i];
		auto pluses = allIncludes(t, plus);
		auto minuses = allIncludes(t, minus);
		intersectAll(plus, pluses, t);
		intersectAll(minus, minuses, t);
		if(i != 0)
			output << ",\n";
		output << "{ \"hypot_plus\": ";
		write_array(pluses, output);
		output << ",\n \"hypot_minus\": ";
		write_array(minuses, output);
	
		output << ",\n\"activity\": \"";
		write_activity(t, attrs, props, output);
		output << "\"\n";
		output << "\n}\n";
	}
	output << "]\n";
}

int main(int argc, char* argv[]){
	ios_base::sync_with_stdio(false);
	int i = 1;
	size_t attrs = 0, props = 0;
	string plus_h, minus_h, tau, json;
	for (; i < argc && argv[i][0] == '-'; i++){
		switch (argv[i][1]){
		case 's':
			attrs = atoi(argv[i] + 2);
			break;
		case 'p':
			props = atoi(argv[i] + 2);
			break;
		case '+':
			plus_h = string(argv[i] + 2);
			break;
		case '-':
			minus_h = string(argv[i] + 2);
			break;
		case 't':
			tau = string(argv[i] + 2);
			break;
		case 'o':
			json = string(argv[i] + 2);
			break;
		default:
			cerr << "Unrecognized option: " << argv[i] << endl;
		}
	}
	if(attrs == 0){
		cerr << "No attributes specified" << endl;
		usage();
	}
	if(props == 0){
		cerr << "No properties specified" << endl;
		usage();
	}
	if (plus_h.empty()){
		cerr << "No plus hypotheses file" << endl;
		usage();
	}
	if (minus_h.empty()){
		cerr << "No minus hypotheses file" << endl;
		usage();
	}
	if (tau.empty()){
		cerr << "No tau examples file given" << endl;
		usage();
	}
	if (json.empty()){
		cerr << "No output file given" << endl;
		usage();
	}
	argv = argv + i;
	argc = argc - i;
	ifstream plus_in(plus_h.c_str());
	ifstream minus_in(minus_h.c_str());
	ifstream tau_in(tau.c_str());
	ofstream json_out(json.c_str());
	auto fp = readFIMI(plus_in, nullptr);
	auto fm = readFIMI(minus_in, nullptr);
	auto ft = readFIMI(tau_in, nullptr);

	Set::setSize(attrs+props);
	Set::Pool storage(fp.size()+fm.size()+ft.size());
	auto plus_sets = toSets(fp, storage);
	auto minus_sets = toSets(fm, storage);
	auto tau_sets = toSets(ft, storage);
	cerr << "PLUS: " << plus_sets.size() << endl;
	cerr << "MINUS: "<<minus_sets.size() << endl;
	cerr << "TAU: "<< tau_sets.size() << endl;
	// Все данные готовы перейти к классификации
	classify(attrs, props, plus_sets, minus_sets, tau_sets, json_out);

	return 0;
}