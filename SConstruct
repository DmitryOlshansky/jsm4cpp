env = Environment()

AddOption('--release', dest='release', action='store_true', help='release build')

release = GetOption('release')

if env['CXX'] == 'cl':
    env.Append(CCFLAGS="/EHsc")
elif env['CXX'] == 'g++' or env['CXX'] == 'clang++':
	if release:
		env.Append(CCFLAGS="-g -Ofast -DNDEBUG -std=c++11 -pthread")
	else:
		env.Append(CCFLAGS="-g -std=c++11 -pthread")
	env.Append(LIBS=["pthread"])

env.Program('gen', ['src/gen.cpp', 'src/sets.cpp'])
env.Program('jsm', ['src/jsm.cpp', 'src/sets.cpp'])
env.Program('jsm_classify', ['src/jsm_classify.cpp', 'src/sets.cpp'])
