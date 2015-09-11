import os

Import('env', 'suffix', 'prefix')
gen = env.Program('gen'+suffix, ['src/gen.cpp', 'src/sets.cpp'])
jsm = env.Program('jsm', ['src/jsm.cpp', 'src/sets.cpp'])
jsm_classify = env.Program('jsm_classify', ['src/jsm_classify.cpp', 'src/sets.cpp'])
Default(gen)
env.Alias('gen', gen) 
env.Alias('jsm', jsm)
env.Alias('jsm', jsm_classify)

env.Alias('install', env.Install(os.path.join(prefix, "bin"), jsm))
env.Alias('install', env.Install(os.path.join(prefix, "bin"), jsm_classify))
