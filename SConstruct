env = Environment()

AddOption('--alloc', dest='alloc', type='string', help='kind of allocator to use for sets')
AddOption('--extent', dest='extent', type='string', help='type of set to use for extents')
AddOption('--intent', dest='intent', type='string', help='type of set to use for intents')
AddOption('--release', dest='release', action='store_true', help='release build')
AddOption('--suffix', dest='suffix', type='string', help='suffix to append to resulting binaries <name>-<suffix>')
AddOption('--writer', dest='writer', type='string', help='type of integer writer to use for I/O')

flags = ""
allocTab = {
	'malloc': "-DUSE_MALLOC_ALLOC",
	'new': "-DUSE_NEW_ALLOC",
	'shared-pool': "-DUSE_SHARED_POOL_ALLOC",
	'tls-pool': "-DUSE_TLS_POOL_ALLOC",
}
extentTab = {
	'linear': "-DUSE_LINEAR_EXT",
	'bitset': "-DUSE_BIT_EXT"
}
intentTab = {
	'linear': "-DUSE_LINEAR_INT",
	'bitset': "-DUSE_BIT_INT"
}
writerTab = {
	'simple' : '-DUSE_SIMPLE_WRITER',
	'table': '-DUSE_TABLE_WRITER'
}

def select(varName, varTab):
	global flags
	var = GetOption(varName)
	if not var in varTab:
		print "Must pass correct --%s parameter, one of %s" % (varName, varTab.keys())
		exit(1)
	else:
		flags += " "+varTab[var]

select('alloc', allocTab)
select('extent', extentTab)
select('intent', intentTab)
select('writer', writerTab)

release = GetOption('release')
suffix = GetOption('suffix')
suffix = "" if suffix == None else "-%s" % suffix

if env['CXX'] == 'cl':
    env.Append(CCFLAGS="/EHsc "+flags)
elif env['CXX'] == 'g++' or env['CXX'] == 'clang++':
	if release:
		env.Append(CCFLAGS="-Ofast -DNDEBUG -std=c++11 -pthread"+flags)
	else:
		env.Append(CCFLAGS="-g -std=c++11 -pthread"+flags)
	env.Append(LIBS=["pthread"])

env.Program('gen'+suffix, ['src/gen.cpp', 'src/sets.cpp'])
env.Program('jsm'+suffix, ['src/jsm.cpp', 'src/sets.cpp'])
env.Program('jsm_classify'+suffix, ['src/jsm_classify.cpp', 'src/sets.cpp'])
