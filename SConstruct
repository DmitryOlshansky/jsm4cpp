env = Environment()

AddOption('--release', dest='release', action='store_true', help='release build')
if env['CXX'] == 'cl':
    env.Append(CCFLAGS="/EHsc")
elif env['CXX'] == 'g++':
	if GetOption('release'):
		env.Append(CCFLAGS="-Ofast -DNDEBUG -std=c++11 -pthread")
	else:
		env.Append(CCFLAGS="-g -std=c++11 -pthread")
	env.Append(LIBS=["pthread"])

env.Program('gen', ['src/gen.cpp'])
env.Program('jsm', ['src/jsm.cpp'])
env.Program('jsm_classify', ['src/jsm_classify.cpp'])
