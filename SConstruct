env = Environment()
if env['CXX'] == 'cl':
    env.Append(CCFLAGS="/EHsc")
elif env['CXX'] == 'g++':
	env.Append(CCFLAGS="-O2 -std=c++11 -pthread -Wl,--no-as-needed ")

env.Program('gen', ['gen.cpp'])
env.Program('jsm', ['jsm.cpp'])
env.Program('jsm_classify', ['jsm_classify.cpp'])