import std.algorithm, core.bitop, std.conv, std.exception, std.math, std.random,
	std.range, std.string, std.stdio, std.traits;

immutable double stochasticError = 1e-3;
immutable maxK = 100; // first Uk-s to compute

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
	density.sort!"a>b"();
	writeln("K\tMaxMUk\tSt.MUk\tMinUk\tMaxMuk^^k\tSt.MUk^^k\tMinUk^^k");
	for(size_t k=1; k<min(maxK, density.length); k++){
		auto maxMu = geometricMean(density[0..k]);
		auto minMu = geometricMean(density[$-k..$]);
		// stochastic geometric mean of k sets, with accepted varience of < 1e-7
		auto stMu = stochasticMean(density, k, stochasticError);
		writefln("%s\t%g\t%g\t%g\t%g\t%g\t%g", k, maxMu, stMu, minMu, pow(maxMu, k), pow(stMu, k), pow(minMu, k));
	}
}

double geometricMean(Range)(Range args) if(isFloatingPoint!(ElementType!Range)){
	//sort!"a > b"(args); // start multiplication with biggest numbers...
	return pow(args.reduce!((a,b) => a*b), 1.0/args.length);
}

double stochasticMean(Range)(Range args, size_t k, double varience) if(isFloatingPoint!(ElementType!Range)){
	double meanOverSamples(size_t samples){
		double accum = 0.0;
		foreach(_; 0..samples){
			accum += geometricMean(randomSample(args, k).array);
		}
		return accum / samples;
	}
	double prev = meanOverSamples(16);
	// double prev_delta = 0;
	for(size_t i=16; i<=2^^25; i *= 2){ // prevent getting stuck forever
		double cur = meanOverSamples(i);
		double delta = fabs(cur - prev);
		if(delta < varience){ // naive numeric code
			// writefln("Converged with 2^^%s samples.", bsr(i));
			return cur;
		}
		prev = cur;
		// prev_delta = delta;
	}
	return double.nan; // not converged
}

void main(string[] args){
	if(args.length == 1)
		return process(stdin, "-");
	foreach(arg; args[1..$])
		process(File(arg), arg);
}