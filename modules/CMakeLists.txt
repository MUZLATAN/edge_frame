cmake_minimum_required(VERSION 3.0)
project(vision-api VERSION 1.0.0)

# add_definitions(-w) # close warning specify the C++ standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
enable_testing()

option(BUILD_FACEDOOR " build for facedoor program " OFF)

if(BUILD_FACEDOOR)
  message(STATUS "NOTE: now is building for facedoor program")
  add_definitions("-D__BUILD_FACEDOOR__")
endif()

include_directories(${CMAKE_CURRENT_LIST_DIR}/api/include
                    ${CMAKE_CURRENT_LIST_DIR}/interfaces)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/api/src/ SRC)

list(
  APPEND
  FACE_LIBS
  glog
  boost_program_options
  opencv_core
  opencv_highgui
  opencv_imgproc
  opencv_imgcodecs
  opencv_videoio
  opencv_calib3d
  opencv_features2d)

if(PLATFORM STREQUAL "hisi")
  # add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/hisi)
  include_directories(${CMAKE_CURRENT_LIST_DIR}/hisi/NNIE/include
                      ${CMAKE_CURRENT_LIST_DIR}/hisi/NNIE/public)
  aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/hisi/NNIE/src/ SRC)
  aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/hisi/NNIE/src/IVE SRC)
  list(
    APPEND
    FACE_LIBS
    mpi
    ive
    md
    nnie
    hdmi
    _hiae
    isp
    _hidehaze
    _hidrc
    _hildci
    _hiawb
    sns_imx327
    sns_imx327_2l
    sns_imx307
    sns_imx458
    sns_mn34220
    sns_os05a
    VoiceEngine
    securec
    sns_imx335
    upvqe
    dnvqe)

else()
  include_directories(${CMAKE_CURRENT_LIST_DIR}/NE/Tengine/include
                      ${CMAKE_CURRENT_LIST_DIR}/FaceDoor/include)
  aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/NE/Tengine/src/ SRC)
  aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/FaceDoor/src/ SRC)
  list(APPEND FACE_LIBS tengine)
endif()

# message("FACE_LIBS is " ${FACE_LIBS}) message("SRC is " ${SRC})

add_library(facesdk SHARED ${SRC})

target_link_libraries(facesdk ${FACE_LIBS})

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/track)

if(PLATFORM STREQUAL "hisi")
  include(${CMAKE_CURRENT_LIST_DIR}/hisi/CMakeLists.txt)
endif()

