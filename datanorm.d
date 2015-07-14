import std.stdio, std.algorithm, std.range, std.conv;

void main(){
	foreach(line; stdin.byLine){
		auto nums = line.split(" ").map!(x=>to!int(x)).array;
		nums.sort();
		foreach(i, n; nums){
			if(i != 0)
				write(" ");
			write(n);
		}
		writeln();
	}
}