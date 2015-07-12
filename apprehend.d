import std.algorithm, std.conv, std.exception, std.math, std.range, std.string, std.stdio;


void process(File inp, string filename){
	size_t[] counts; // aggregated object counts per attribute
	size_t objects;
	foreach(line; inp.byLine){
		auto p = line;
		try{
			bool firstTime = true;
			for(;;){
				munch(p, " \t");
				if(p.empty)
					break;
				auto attr = parse!size_t(p);
				if(counts.length <= attr)
					counts.length = attr+1;
				counts[attr]++;
				if(firstTime){ // at least 1 attribute parsed
					firstTime = false;  
					objects++; //it's an object not blank line
				}
			}
		}
		catch(ConvException){} // just get out of loop
	}
	if(objects == 0){
		writefln("No objects found in %s.", filename);
		return;
	}
	foreach(i,c; counts)  // not greater then total number of objects
		enforce(c <= objects, "Bad formal context - attribute that contains duplicates in same row: " ~ to!string(i));
	auto density = counts
		.filter!(x => x!= 0 && x != objects)
		.map!(x => cast(double)x / objects)
		.array; // 0<..<1 density array
	writeln("K\tUk");
	for(size_t k=1; k<min(13,density.length); k++){
		auto m = maxGeoMeanByK(density, k);
		writefln("%s\t%s", k, m);
	}
}

double maxGeoMeanByK(double[] args, size_t k){
	auto permut = iota(0, k).array;
	double maxSoFar = 0;
	do{
		// use the current permutation and
		auto gmean = permut.map!(x => args[x]).array.geometricMean;
		if(gmean > maxSoFar)
			maxSoFar = gmean;
		// proceed to the next permutation of the array.
	}while(nextPermutation(permut));
	return maxSoFar;
}

double geometricMean(Range)(Range args){
	sort!"a > b"(args); // start multiplication with biggest numbers...
	return pow(args.reduce!((a,b) => a*b), 1.0/args.length);
}

void main(string[] args){
	if(args.length == 1)
		return process(stdin, "-");
	foreach(arg; args[1..$])
		process(File(arg), arg);
}