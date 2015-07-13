module merge_csv;
import std.algorithm, std.csv, std.exception, std.getopt,
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
			foreach(r; ranges) r.popFront();
		}
	}
	return LockStepArray!Range(ranges);
}

void main(string[] args){
	try{
		auto scanner = args[1..$].
			map!(x => std.file.readText(x)).
			map!(x => csvReader(x)).array.
			lockstepArray();

		foreach(records; scanner){ // now we scan all files in lockstep

		}
	}
	catch(Exception e){
		stderr.writeln(e);
	}
}
