cmake_minimum_required(VERSION 3.0)
project(vision-edge VERSION 1.0.0)

# add_definitions(-w) # close warning

# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_VERBOSE_MAKEFILE  ON)
enable_testing()
set(CMAKE_SYSTEM_NAME Linux)

find_library(BOOSTTEST_LIBRARY NAMES boost_unit_test_framework)

# add_compile_options(-fPIC)
option(WITH_HEAPPROFILE " build with heap profile " OFF)
option(WITH_GTESTS "build unit test " OFF)

list(
  APPEND
  REQUEST_LIBS

  glog
  folly
  boost_thread
  event
  boost_context
  double-conversion
  unwind
  dl
  iberty
  lzma # folly
  pthread # folly depend on pthread, need put it down
  opencv_core
  opencv_imgproc
  opencv_highgui
  opencv_imgcodecs
  opencv_videoio
  opencv_video
  opencv_calib3d
  opencv_features2d
  boost_program_options
  boost_filesystem
  boost_serialization
  boost_wserialization
  boost_system
  boost_regex # micro cpprest
  x264
  ssl
  crypto # cpprest
  curl
  z
  m
  dl
  cpprest
  uuid
)



include_directories(${CMAKE_CURRENT_LIST_DIR}/modules/interfaces)

if(PLATFORM STREQUAL "x86")
  set(ALGO_3RDPARTY_PATH /env/platforms/x86_64)

  message(STATUS "NOTE: ALGO_3RDPARTY_PATH is: " ${ALGO_3RDPARTY_PATH})
  include_directories(${ALGO_3RDPARTY_PATH}/include/)
  include_directories(/home/z/software/x86/include)
  link_directories(${ALGO_3RDPARTY_PATH}/lib 
                   ${ALGO_3RDPARTY_PATH}/static 
                   ${ALGO_3RDPARTY_PATH}/so )
  link_directories(/home/z/software/x86/lib)

  list(APPEND REQUEST_LIBS jsoncpp)
  add_definitions("-D__PLATFORM_X86__")


  aux_source_directory(${CMAKE_SOURCE_DIR}/src/recognition/ ALGO_SRC)

else()
  message(
    FATAL_ERROR
      "ERROR: you must set PLATFORM value, just like 'arm' 'x86' 'hisi'.")
endif()
message(STATUS "NOTE: PLATFORM is: " ${PLATFORM})

add_subdirectory(${CMAKE_SOURCE_DIR}/modules/)
include_directories(${CMAKE_SOURCE_DIR}/modules/track/include
                    ${CMAKE_SOURCE_DIR}/modules/FaceDoor/include)
list(APPEND 3rdlib "track" "facesdk")

message(STATUS "NOTE: 3rd lib used is: " ${3rdlib})

include_directories(${CMAKE_SOURCE_DIR}/include)

aux_source_directory(${CMAKE_SOURCE_DIR}/src/ ALGO_SRC)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/event ALGO_SRC)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/network ALGO_SRC)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/processor ALGO_SRC)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/mgr ALGO_SRC)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/app ALGO_SRC)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/lib ALGO_SRC)

set(BIN edge)
add_executable(${BIN} ${ALGO_SRC})
target_link_libraries(${BIN} ${3rdlib} ${REQUEST_LIBS})

