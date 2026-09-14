cmake_minimum_required(VERSION 3.16)
project(CSIdumpV1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(csi_protocol_v1 STATIC
    src/csi_protocol_v1.cpp
)
target_include_directories(csi_protocol_v1 PUBLIC include)
target_compile_options(csi_protocol_v1 PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(protocol_v1_test tests/protocol_v1_test.cpp)
target_link_libraries(protocol_v1_test PRIVATE csi_protocol_v1)
target_compile_options(protocol_v1_test PRIVATE -Wall -Wextra -Wpedantic -Werror)

enable_testing()
add_test(NAME protocol_v1_test COMMAND protocol_v1_test)

option(BUILD_ROUTER_BINARY "Build the OpenWrt/mt76 CSIdumpV1 executable" OFF)
if(BUILD_ROUTER_BINARY)
    find_path(UNL_INCLUDE_DIR unl.h REQUIRED)
    find_library(NL_TINY_LIBRARY nl-tiny REQUIRED)
    find_package(Threads REQUIRED)

    add_executable(CSIdumpV1
        src/main.cpp
        src/csi_server_v1.cpp
        src/mt76_csi_frame.cpp
    )
    target_include_directories(CSIdumpV1 PRIVATE include ${UNL_INCLUDE_DIR})
    target_link_libraries(CSIdumpV1 PRIVATE csi_protocol_v1 ${NL_TINY_LIBRARY} Threads::Threads)
    target_compile_options(CSIdumpV1 PRIVATE -Wall -Wextra -Wpedantic)
endif()
