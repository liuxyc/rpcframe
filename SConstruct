import os
#gen rpcframe proto
os.system("LD_LIBRARY_PATH=./thirdparty/protobuf-261/lib/ thirdparty/protobuf-261/bin/protoc --cpp_out=common/proto/ -Icommon/proto common/proto/rpc.proto")

env = Environment(CCFLAGS = '-std=c++11 -g -Wall -O3 -D_GLIBCXX_USE_CXX11_ABI=0')
mongoose_env = Environment(CCFLAGS = '-g -Wall -DMONGOOSE_ENABLE_THREADS', CPPPATH = ['server/include/'])

mongoose_obj = mongoose_env.Object(Glob('server/src/*.c'))

#rpcframe static library
rpcframe_src_files = Glob('server/src/*.cpp') 
rpcframe_src_files.append(Glob('client/src/*.cpp'))
rpcframe_src_files.append('common/proto/rpc.pb.cc')
rpcframe_src_files.append(Glob('common/src/*.cpp'))
env.StaticLibrary('rpcframe', rpcframe_src_files + mongoose_obj, 
    CPPPATH = ['thirdparty/protobuf-261/include/', 'server/include/', 'client/include', 'common/include', 'common/proto'],
    LIBS=['pthread'])

env.Install('./output/include/', 'client/include/RpcClient.h')
env.Install('./output/include/', 'common/include/RpcDefs.h')
env.Install('./output/include/', 'server/include/RpcServer.h')
env.Install('./output/include/', 'server/include/IRpcRespBroker.h')
env.Install('./output/include/', 'server/include/IService.h')
env.Install('./output/lib/', 'librpcframe.a')
Clean('', './output')

#client_test
client_test_src = Split('test/client_test.cpp')
env.Program('client_test', client_test_src, 
    LIBS=['rpcframe', 'pthread', 'uuid', 'protobuf'], 
    LIBPATH = ['.', 'thirdparty/protobuf-261/lib/'], 
    CPPPATH = ['output/include/'])


#server_test
server_test_src = Split('test/server_test.cpp')
env.Program('server_test', server_test_src, 
    LDFLAGS=[''], 
    LIBS=['rpcframe', 'pthread', 'protobuf', 'profiler'], 
    #LIBS=['rpcframe', 'pthread', 'protobuf'], 
    #LINKFLAGS=['-Wl,--no-as-needed'],
    LIBPATH = ['.', 'thirdparty/protobuf-261/lib/'], 
    CPPPATH = ['output/include/'])

#queue_test
env.Program('queue_test', 'test/queue_test.cpp', 
    LIBS=['rpcframe', 'gtest_main', 'gtest', 'pthread'], \
    LIBPATH=['.', 'thirdparty/gtest/lib'], 
    CPPPATH=['common/include', 'thirdparty/gtest/include'])
