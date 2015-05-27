env = Environment(CCFLAGS = '-std=c++11 -g -Wall')

rpcframe_src_files = Glob('server/src/*.cpp') 
rpcframe_src_files.append(Glob('client/src/*.cpp'))
rpcframe_src_files.append('common/src/rpc.pb.cc')
env.StaticLibrary('rpcframe', rpcframe_src_files, CPPPATH = ['server/include/', 'client/include', 'common/include'], LIBS=['pthread'])

client_test_src = Split('test/client_test.cpp')
env.Program('client_test', client_test_src, LIBS=['rpcframe', 'pthread', 'uuid', 'protobuf'], LIBPATH='.', CPPPATH = ['./include', 'common/include', 'client/include'])

server_test_src = Split('test/server_test.cpp')
env.Program('server_test', server_test_src, LDFLAGS=[''], LIBS=['rpcframe', 'protobuf', 'pthread'], LIBPATH='.', CPPPATH = ['server/include', 'common/include'])

env.Program('queue_test', 'test/queue_test.cpp', LIBS=['rpcframe', 'gtest_main', 'gtest', 'pthread'], \
    LIBPATH=['.', './gtest/lib'], CPPPATH=['common/include', './gtest/include'])
