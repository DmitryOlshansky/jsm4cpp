env = Environment()

AddOption('--release', dest='release', action='store_true', help='release build')
AddOption('--extent', dest='extent', type='string', help='type of set to use for extents')
AddOption('--intent', dest='intent', type='string', help='type of set to use for intents')
AddOption('--writer', dest='writer', type='string', help='type of integer writer to use for I/O')

flags = ""

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

select('extent', extentTab)
select('intent', intentTab)
select('writer', writerTab)

release = GetOption('release')

if env['CXX'] == 'cl':
    env.Append(CCFLAGS="/EHsc "+flags)
elif env['CXX'] == 'g++' or env['CXX'] == 'clang++':
	if release:
		env.Append(CCFLAGS="-Ofast -DNDEBUG -std=c++11 -pthread"+flags)
	else:
		env.Append(CCFLAGS="-g -std=c++11 -pthread"+flags)
	env.Append(LIBS=["pthread"])

env.Program('gen', ['src/gen.cpp', 'src/sets.cpp'])
env.Program('jsm', ['src/jsm.cpp', 'src/sets.cpp'])
env.Program('jsm_classify', ['src/jsm_classify.cpp', 'src/sets.cpp'])
