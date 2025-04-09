cmake_minimum_required(VERSION 3.17)

project(ElasticTask)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/tools/cmake/")

# Search for SimGrid
#set(SimGrid_DIR "/home/clem/Code/framagit.org/simgridOfficiel/simgrid/build/" CACHE PATH "Simgrid path")

find_package(SimGrid REQUIRED)
include_directories("${SimGrid_INCLUDE_DIR}" SYSTEM)

MESSAGE("${SimGrid_INCLUDE_DIR}")

if(DEFINED ${USE_JAEGER})
    option(USE_JAEGERTRACING "Use jaeger traces through opentracing" ON)
    configure_file(Elasticity/ElasticConfig.hpp.in ${PROJECT_SOURCE_DIR}/Elasticity/ElasticConfig.hpp)
endif()
    
add_subdirectory(Elasticity)
add_subdirectory(examples)
