cmake_minimum_required(VERSION 3.10)
project(MarketDataGateway)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf REQUIRED)

include_directories(include)
include_directories(src)
include_directories(third_party/ZeroNet/include)
include_directories(${Protobuf_INCLUDE_DIRS})

file(GLOB ZERONET_SOURCES "third_party/ZeroNet/src/*.cpp")

add_executable(GatewayApp
        src/main.cpp
        src/SubscriberManager.cpp
        src/UpstreamReceiver.cpp
        src/subscription.pb.cc
        ${ZERONET_SOURCES}
)

target_link_libraries(GatewayApp ${Protobuf_LIBRARIES} pthread)
