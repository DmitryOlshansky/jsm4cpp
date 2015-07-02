env = Environment()

AddOption('--release', dest='release', action='store_true', help='release build')
AddOption('--extent', dest='extent', type='string', help='type of set to use for extents')

flags = ""
extent = GetOption('extent')
if extent == 'linear':
	flags += "-DUSE_LINEAR_EXT"
elif extent == 'bitset':
	flags += "-DUSE_BIT_EXT"
else:
	print "Must pass correct --extent parameter!"
	exit(1)
release = GetOption('release')

if env['CXX'] == 'cl':
    env.Append(CCFLAGS="/EHsc "+flags)
elif env['CXX'] == 'g++' or env['CXX'] == 'clang++':
	if release:
		env.Append(CCFLAGS="-g -Ofast -DNDEBUG -std=c++11 -pthread "+flags)
	else:
		env.Append(CCFLAGS="-g -std=c++11 -pthread "+flags)
	env.Append(LIBS=["pthread"])

env.Program('gen', ['src/gen.cpp', 'src/sets.cpp'])
env.Program('jsm', ['src/jsm.cpp', 'src/sets.cpp'])
env.Program('jsm_classify', ['src/jsm_classify.cpp', 'src/sets.cpp'])
