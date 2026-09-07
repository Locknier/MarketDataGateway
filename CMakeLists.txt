cmake_minimum_required(VERSION 3.10)
project(MarketDataGateway)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf REQUIRED)

include_directories(include)
include_directories(src)
include_directories(third_party/ZeroNet/include)
include_directories(third_party/ctp_api/include)
include_directories(${Protobuf_INCLUDE_DIRS})

# 将 CTP 动态库所在目录添加到链接器寻找路径
link_directories(${CMAKE_SOURCE_DIR}/third_party/ctp_api/lib)

file(GLOB ZERONET_SOURCES "third_party/ZeroNet/src/*.cpp")

add_executable(GatewayApp
        src/main.cpp
        src/SubscriberManager.cpp
        src/CtpMdReceiver.cpp
        src/subscription.pb.cc
        ${ZERONET_SOURCES}
)

# 动态链接 CTP 引擎 (-lthostmduserapi_se 代表 libthostmduserapi_se.so)
target_link_libraries(GatewayApp ${Protobuf_LIBRARIES} thostmduserapi_se pthread)
