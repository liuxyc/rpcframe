PROJECT(rpcframe)  
cmake_minimum_required(VERSION 2.8)  

SET(SERVER_DIR ${PROJECT_SOURCE_DIR}/server)
SET(CLIENT_DIR ${PROJECT_SOURCE_DIR}/client)
SET(COMMON_DIR ${PROJECT_SOURCE_DIR}/common)
SET(TEST_DIR ${PROJECT_SOURCE_DIR}/test)
SET(OUTPUT_DIR ${PROJECT_SOURCE_DIR}/output)
SET(PROTO_DIR ${PROJECT_SOURCE_DIR}/common/proto)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g3 -Wall -O0")  
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -Wall -O0")
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

EXECUTE_PROCESS(
  COMMAND rm -f ${PROTO_DIR}/*.cc ${PROTO_DIR}/*.h
  COMMAND protoc -I=${PROTO_DIR} --cpp_out=${PROTO_DIR}
  ${PROTO_DIR}/rpc.proto
  WORKING_DIRECTORY  ${PROTO_DIR})

FILE(GLOB rpcframe_src ${SERVER_DIR}/src/*.cpp ${CLIENT_DIR}/src/*.cpp ${COMMON_DIR}/src/*.cpp ${COMMON_DIR}/src/*.c ${PROTO_DIR}/*.cc)
SET(mongoose_src ${SERVER_DIR}/src/mongoose.c)

SET_SOURCE_FILES_PROPERTIES(${rpcframe_src} PROPERTIES 
  LANGUAGE CXX   
  COMPILE_DEFINITIONS MG_ENABLE_THREADS
)
SET_SOURCE_FILES_PROPERTIES(${mongoose_src} PROPERTIES 
  LANGUAGE C
  COMPILE_DEFINITIONS MG_ENABLE_THREADS
)

INCLUDE_DIRECTORIES(  
  ${CLIENT_DIR}/include
  ${SERVER_DIR}/include
  ${COMMON_DIR}/include
  ${COMMON_DIR}/proto
  )  

LINK_DIRECTORIES(  
  /usr/src/gtest
  )  

ADD_LIBRARY(rpcframe STATIC ${rpcframe_src} ${mongoose_src})

SET(PUB_HDR 
  ${CLIENT_DIR}/include/RpcClient.h 
  ${COMMON_DIR}/include/RpcDefs.h 
  ${SERVER_DIR}/include/RpcServer.h 
  ${SERVER_DIR}/include/RpcServerConfig.h 
  ${SERVER_DIR}/include/IService.h
  ${SERVER_DIR}/include/IRpcRespBroker.h)

ADD_CUSTOM_COMMAND(TARGET rpcframe
  PRE_BUILD
  COMMAND rm -rf ${OUTPUT_DIR}
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
ADD_CUSTOM_COMMAND(TARGET rpcframe
  POST_BUILD
  COMMAND mkdir -p ${OUTPUT_DIR}/include ${OUTPUT_DIR}/lib
  COMMAND cp -arf ${PUB_HDR} ${OUTPUT_DIR}/include
  COMMAND cp -arf $<TARGET_FILE:rpcframe> ${OUTPUT_DIR}/lib
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )

#INSTALL (${PUB_HDR} PUBLIC_HEADER DESTINATION include)
#INSTALL (${PROJECT_SOURCE_DIR}/librpcframe.a ARCHIVE DESTINATION lib)

ADD_EXECUTABLE(common_test ${TEST_DIR}/common_test.cpp)
TARGET_LINK_LIBRARIES(common_test rpcframe gtest gtest_main protobuf pthread)

ADD_EXECUTABLE(server_test ${TEST_DIR}/server_test.cpp)
#set (CMAKE_EXE_LINKER_FLAGS "-Wl,--no-as-needed")
#TARGET_LINK_LIBRARIES(server_test rpcframe protobuf pthread profiler)
TARGET_LINK_LIBRARIES(server_test rpcframe protobuf pthread)

ADD_EXECUTABLE(client_test ${TEST_DIR}/client_test.cpp)
#set (CMAKE_EXE_LINKER_FLAGS "-Wl,--no-as-needed")
#TARGET_LINK_LIBRARIES(client_test rpcframe protobuf pthread gtest gtest_main profiler)
TARGET_LINK_LIBRARIES(client_test rpcframe protobuf pthread gtest gtest_main)
