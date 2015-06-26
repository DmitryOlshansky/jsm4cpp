env = Environment()
# release = 
if env['CXX'] == 'cl':
    env.Append(CCFLAGS="/EHsc")
elif env['CXX'] == 'g++':
	env.Append(CCFLAGS="-g -std=c++11 -pthread -Wl,--no-as-needed ")

env.Program('gen', ['src/gen.cpp'])
env.Program('jsm', ['src/jsm.cpp'])
env.Program('jsm_classify', ['src/jsm_classify.cpp'])
