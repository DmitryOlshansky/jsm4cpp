env = Environment()

AddOption('--alloc', dest='alloc', type='string', help='kind of allocator to use for sets')
AddOption('--extent', dest='extent', type='string', help='type of set to use for extents')
AddOption('--intent', dest='intent', type='string', help='type of set to use for intents')
AddOption('--release', dest='release', action='store_true', help='release build')
AddOption('--suffix', dest='suffix', type='string', help='override a suffix to append to resulting binaries <name>-<suffix>')
AddOption('--writer', dest='writer', type='string', help='type of integer writer to use for I/O')
AddOption('--cxx', dest='cxx', type='string', help='c++ compiler to use')
flags = ""
libs = []
var_name=""
allocTab = {
	'malloc': "-DUSE_MALLOC_ALLOC",
	'new': "-DUSE_NEW_ALLOC",
	'shared-pool': "-DUSE_SHARED_POOL_ALLOC",
	'tls-pool': "-DUSE_TLS_POOL_ALLOC",
}
extentTab = {
	'linear': "-DUSE_LINEAR_EXT",
	'bitset': "-DUSE_BIT_EXT",
	'tree': '-DUSE_TREE_EXT'
}
intentTab = {
	'linear': "-DUSE_LINEAR_INT",
	'bitset': "-DUSE_BIT_INT",
	'tree': '-DUSE_TREE_INT'
}
writerTab = {
	'simple' : '-DUSE_SIMPLE_WRITER',
	'table': '-DUSE_TABLE_WRITER'
}

def select(varName, varTab):
	global flags, var_name
	var = GetOption(varName)
	if not var in varTab:
		print "Must pass correct --%s parameter, one of %s" % (varName, varTab.keys())
		exit(1)
	else:
		flags += " "+varTab[var]
		var_name += "-" + (var[0:1] if varName == 'alloc' else var[0:3])

select('extent', extentTab)
select('intent', intentTab)
select('writer', writerTab)
select('alloc', allocTab)
alloc = GetOption('alloc')
if  alloc == 'tls-pool' or alloc == 'shared-pool':
	libs = ["boost_system"]
cxx = GetOption('cxx')
if cxx: # C++ compiler override
	env['CXX'] = cxx
release = GetOption('release')
suffix = GetOption('suffix')
suffix = var_name if suffix == None else "-%s" % suffix

if env['CXX'] == 'cl':
    env.Append(CCFLAGS="/EHsc "+flags)
elif 'g++' in env['CXX'] or 'clang++' in env['CXX']:
	if release:
		env.Append(CCFLAGS="-Ofast -DNDEBUG -std=c++11 -pthread"+flags)
	else:
		env.Append(CCFLAGS="-g -std=c++11 -pthread"+flags)
	env.Append(LIBS=["pthread"]+libs)

SConscript(dirs='.', variant_dir="build/stage"+var_name, duplicate=False, exports=["env", "suffix"])