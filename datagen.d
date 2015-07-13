import std.math, std.random, std.stdio, std.conv, std.range, std.array;

int main(string[] argv){
    if(argv.length  < 4){
        stderr.writeln("Usage ./data-gen <objects> <attributes> <ratio>");
        return 1;
    }

    int objects = to!int(argv[1]);
    int attributes = to!int(argv[2]);
    double ratio = to!double(argv[3]);
    int[] numbers = iota(0, attributes).array;
    foreach(_; 0..objects){
        int cover = cast(int)ceil(attributes*ratio);
        writefln("%( %d %)", randomSample(numbers, cover));
    }
    return 0;
}