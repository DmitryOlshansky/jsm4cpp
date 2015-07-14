module merge_csv;
import std.algorithm, std.csv, std.conv, std.exception, std.getopt,
	std.range, std.stdio, std.traits;

static import std.file;

auto lockstepArray(Range)(Range[] ranges){
	static struct LockStepArray(Range){
		Range[] ranges;
		
		@property auto front(){
			return ranges.map!(x=>x.front);
		}

		@property bool empty(){
			return ranges.any!(x=>x.empty); // stop on first empty
		}

		void popFront(){
			foreach(ref r; ranges) r.popFront();
		}
	}
	return LockStepArray!Range(ranges);
}

enum Operation{
	mean,
	min
}

void main(string[] args){
	Operation op = Operation.min;
	auto ret = getopt(args, "op", "operation for merging: mean or min", &op);
	if(ret.helpWanted){
		return defaultGetoptPrinter("Merge CSV.", ret.options);
	}
	try{
		auto scanner = args[1..$].
			map!(x => std.file.readText(x)).
			map!(x => csvReader!(string, Malformed.ignore)(x)).array.
			lockstepArray();
		bool header = true;
		foreach(records; scanner){ // now we scan all files in lockstep
			if(header){
				writefln("%(%s,%)", records[0]); //just pick the first one
				header = false;
				continue;
			}
			// turn each record into array
			auto data = records.
				map!(x=>x.map!(y=> y.length ? to!double(y) : 0)).
				map!(z=>z.array).array;
			double[] merged = new double[data[0].length]; //TODO: pick min or max?
			foreach(i; 0..merged.length){
				if(op == Operation.mean){
					double accum = 0.0;
					foreach(j;0..data.length)
						accum += data[j][i];
					merged[i] = accum/data.length;
				}
				else if(op == Operation.min){
					double m = double.max;
					foreach(j;0..data.length)
						if(data[j][i] < m)
							m = data[j][i];
					merged[i] = m;
				}
			}
			writefln("%(%s,%)", merged);
		}
	}
	catch(Exception e){
		stderr.writeln(e);
	}
}
