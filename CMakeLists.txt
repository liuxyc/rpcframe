#项目名称  
PROJECT(rpcframe)  
#声明要求的最小版本  
cmake_minimum_required(VERSION 3.0)  

SET(SERVER_DIR ${PROJECT_SOURCE_DIR}/server)
SET(CLIENT_DIR ${PROJECT_SOURCE_DIR}/client)
SET(COMMON_DIR ${PROJECT_SOURCE_DIR}/common)
SET(TEST_DIR ${PROJECT_SOURCE_DIR}/test)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -O3")  
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -O3")  

#添加参与编译的子目录 我理解为：添加下一个CMakeLists.txt  
EXECUTE_PROCESS(COMMAND protoc -I=${COMMON_DIR}/proto --cpp_out=${COMMON_DIR}/proto 
  ${COMMON_DIR}/proto/rpc.proto
  WORKING_DIRECTORY  ${COMMON_DIR}/proto)

FILE(GLOB rpcframe_src ${SERVER_DIR}/src/*.cpp ${CLIENT_DIR}/src/*.cpp ${COMMON_DIR}/src/*.cpp ${COMMON_DIR}/proto/*.cc)
SET(mongoose_src ${SERVER_DIR}/src/mongoose.c)

SET_SOURCE_FILES_PROPERTIES(${rpcframe_src} PROPERTIES 
  LANGUAGE CXX   
  COMPILE_DEFINITIONS MG_ENABLE_THREADS
)
SET_SOURCE_FILES_PROPERTIES(${mongoose_src} PROPERTIES 
  LANGUAGE C
  COMPILE_DEFINITIONS MG_ENABLE_THREADS
)

#一定要包含main.cpp 包含的"*.h"文件  
INCLUDE_DIRECTORIES(  
  ${CLIENT_DIR}/include
  ${SERVER_DIR}/include
  ${COMMON_DIR}/include
  ${COMMON_DIR}/proto
  )  
#库文件路径  
LINK_DIRECTORIES(  
  /usr/src/gtest
  )  

ADD_LIBRARY(rpcframe STATIC ${rpcframe_src} ${mongoose_src})

SET(PUB_HDR ${CLIENT_DIR}/include/RpcClient.h common/include/RpcDefs.h ${SERVER_DIR}/include/RpcServer.h ${SERVER_DIR}/include/IRpcRespBroker.h)

ADD_CUSTOM_COMMAND(TARGET rpcframe
  PRE_BUILD
  COMMAND rm -rf ${PROJECT_SOURCE_DIR}/output
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
ADD_CUSTOM_COMMAND(TARGET rpcframe
  POST_BUILD
  COMMAND mkdir -p ${PROJECT_SOURCE_DIR}/output/include ${PROJECT_SOURCE_DIR}/output/lib
  COMMAND cp -arf ${PUB_HDR} ${PROJECT_SOURCE_DIR}/output/include
  COMMAND cp -arf ${PROJECT_SOURCE_DIR}/librpcframe.a ${PROJECT_SOURCE_DIR}/output/lib
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )

#INSTALL (${PUB_HDR} PUBLIC_HEADER DESTINATION include)
#INSTALL (${PROJECT_SOURCE_DIR}/librpcframe.a ARCHIVE DESTINATION lib)

ADD_EXECUTABLE(queue_test ${TEST_DIR}/queue_test.cpp)
TARGET_LINK_LIBRARIES(queue_test rpcframe gtest gtest_main protobuf pthread)

ADD_EXECUTABLE(server_test ${TEST_DIR}/server_test.cpp)
TARGET_LINK_LIBRARIES(server_test rpcframe protobuf pthread)

ADD_EXECUTABLE(client_test ${TEST_DIR}/client_test.cpp)
TARGET_LINK_LIBRARIES(client_test rpcframe protobuf pthread)
