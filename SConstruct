env = Environment(CCFLAGS = '-std=c++11 -g -Wall -O3')

rpcframe_src_files = Glob('server/src/*.cpp') 
rpcframe_src_files.append(Glob('client/src/*.cpp'))
rpcframe_src_files.append('common/src/rpc.pb.cc')
rpcframe_src_files.append(Glob('common/src/*.cpp'))
env.StaticLibrary('rpcframe', rpcframe_src_files, 
    CPPPATH = ['thirdparty/protobuf-261/include/', 'server/include/', 'client/include', 'common/include'],
    LIBS=['pthread'])

client_test_src = Split('test/client_test.cpp')
env.Program('client_test', client_test_src, 
    LIBS=['rpcframe', 'pthread', 'uuid', 'protobuf'], 
    LIBPATH = ['.', 'thirdparty/protobuf-261/lib/'], 
    CPPPATH = ['./include', 'common/include', 'client/include'])

server_test_src = Split('test/server_test.cpp')
env.Program('server_test', server_test_src, 
    LDFLAGS=[''], 
    LIBS=['rpcframe', 'pthread', 'protobuf'], 
    LIBPATH = ['.', 'thirdparty/protobuf-261/lib/'], 
    CPPPATH = ['server/include', 'common/include'])

env.Program('queue_test', 'test/queue_test.cpp', 
    LIBS=['rpcframe', 'gtest_main', 'gtest', 'pthread'], \
    LIBPATH=['.', 'thirdparty/gtest/lib'], 
    CPPPATH=['common/include', 'thirdparty/gtest/include'])
