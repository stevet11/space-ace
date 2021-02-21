cmake_minimum_required(VERSION 3.0)
project(spaceace
  VERSION 0.1
  LANGUAGES CXX C)

###########
# Debug or Release
###########
if (NOT CMAKE_BUILD_TYPE)
  ## set default to Debug
  set(CMAKE_BUILD_TYPE Debug)  # override with -DCMAKE_BUILD_TYPE=Release
  message("==> CMAKE_BUILD_TYPE empty. Changing it to Debug.")
else()
  message("==> CMAKE_BUILD_TYPE == ${CMAKE_BUILD_TYPE}.")
endif()

## https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_macros.html
## During Debug, use debug version of libstdc++ (asserts on access to invalid iterators, etc!)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

##############
# C++ Standard
##############
set(CMAKE_CXX_STANDARD   14)
# set(CMAKE_CXX_STANDARD   17)
set(CMAKE_CXX_EXTENSIONS ON)

add_subdirectory(door++)

# hahaha.  Why am I not surprised to see glog not wanting to work AT ALL.
#
# add_subdirectory(glog)
# find_package (glog 0.5.0 REQUIRED)

file(GLOB IMAGES
    ansi/*.ans
)

add_custom_command(
    OUTPUT images.h
    COMMAND ./build_images.sh
    DEPENDS ${IMAGES}
    COMMENT "Generating images.h"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

# add_executable(space-ace main.cpp deck.cpp images.h glog::glog)
add_executable(space-ace main.cpp deck.cpp images.h)
target_link_libraries(space-ace door++ pthread)

