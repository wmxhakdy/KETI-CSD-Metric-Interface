# Copyright 2021 the gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# cmake build file for C++ keyvaluestore example.
# Assumes protobuf and gRPC have been installed using cmake.
# See cmake_externalproject/CMakeLists.txt for all-in-one cmake build
# that automatically builds all the dependencies before building keyvaluestore.

cmake_minimum_required(VERSION 3.5.1)

project(CSDMetricCollector C CXX)

include(./cmake/common.cmake)
set(CMAKE_CXX_STANDARD 17)

# Proto file
get_filename_component(mc_proto "./proto/csd-to-metric-collector-grpc.proto" ABSOLUTE)
get_filename_component(mc_proto_path "${mc_proto}" PATH)

# Generated sources
set(mc_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/csd-to-metric-collector-grpc.pb.cc")
set(mc_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/csd-to-metric-collector-grpc.pb.h")
set(mc_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/csd-to-metric-collector-grpc.grpc.pb.cc")
set(mc_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/csd-to-metric-collector-grpc.grpc.pb.h")
add_custom_command(
      OUTPUT "${mc_proto_srcs}" "${mc_proto_hdrs}" "${mc_grpc_srcs}" "${mc_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${mc_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${mc_proto}"
      DEPENDS "${mc_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# # Include common include directories
# include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include")

# kvs_grpc_proto
add_library(mc_grpc_proto
  ${mc_grpc_srcs}
  ${mc_grpc_hdrs}
  ${mc_proto_srcs}
  ${mc_proto_hdrs})
target_link_libraries(mc_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

if (UNIX)
	# Linking POSIX threads
	find_package(Threads REQUIRED)
	set(LINKING_LIBRARIES ${LINKING_LIBRARIES} Threads::Threads)
endif (UNIX)
find_package(CURL REQUIRED)

# merging_container
add_executable(CSDMetricInfoClient "csd-metric-interface-grpc.cc" ) 
target_link_libraries(CSDMetricInfoClient
  mc_grpc_proto
  ${LINKING_LIBRARIES}
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
  CURL::libcurl)
