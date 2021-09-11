cmake_minimum_required(VERSION 3.0)
project(vision-track VERSION 1.0.0)

# specify the C++ standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add_compile_options(-fPIC)

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

# include_directories(/env/platforms/aarch64/include/)
# link_directories(/env/platforms/aarch64/so/ /env/platforms/aarch64/static/)

aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/src TRACK_SRCS)

add_library(track SHARED ${TRACK_SRCS})
target_link_libraries(track gflags glog boost_system)
