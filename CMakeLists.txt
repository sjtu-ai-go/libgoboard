cmake_minimum_required(VERSION 3.1)
project(libgoboard)

option(libgoboard_build_tests "Build libgoboard's own tests" OFF)

set(CMAKE_CXX_STANDARD 11)


#################################
# FindProtobuf
#################################
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})

#################################
# CompressedGrid
#################################
include_directories(vendor/CompressedGrid)

##################################
# libgo-common
##################################
if (NOT TARGET go-common)
    add_subdirectory(vendor/libgo-common)
endif()
set(libgo-common_SOURCE_DIR vendor/libgo-common)
set(libgo_LIBS go-common)
include_directories(${libgo-common_INCLUDE_DIR})

##################################
# libgoboard
##################################
include_directories(src/)
set(libgoboard_SRC src/board.cpp src/message/message.pb.cc)
add_library(goboard STATIC ${libgoboard_SRC})
target_link_libraries(goboard ${libgo_LIBS} ${PROTOBUF_LIBRARIES})
set(libgoboard_INCLUDE_DIR ${libgoboard_SOURCE_DIR}/src ${libgo-common_INCLUDE_DIR} ${libgoboard_SOURCE_DIR}/vendor/CompressedGrid ${Protobuf_INCLUDE_DIRS} PARENT_SCOPE)

#################################
# tests
################################
if (libgoboard_build_tests)
    enable_testing()
    ################################
    # GTest
    ################################
    ADD_SUBDIRECTORY (vendor/googletest/googletest)
    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
    ###############################
    # board-test
    ###############################
    add_executable(board-test src/board_test.cpp)
    target_link_libraries(board-test goboard gtest gtest_main)
    add_test(board_test board-test)
endif()
