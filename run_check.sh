find ./ -name "*.cpp" | xargs cppcheck --std=c++11 --platform=unix64 --enable=all --inconclusive -Iclient/include -Icommon/include -Iserver/include -i rpc.pb.cc rpc.pb.h 
