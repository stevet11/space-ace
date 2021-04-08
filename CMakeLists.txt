cmake_minimum_required(VERSION 3.0)
project(spaceace
  VERSION 0.1
  LANGUAGES CXX C)

###########
# Debug or Release
###########
if (NOT CMAKE_BUILD_TYPE)
  ## set default to Debug
  set(CMAKE_BUILD_TYPE RelWithDebInfo)  # override with -DCMAKE_BUILD_TYPE=Release
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

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/door++)
  message("***")
  message("*** ERROR/MISSING *** please run: git clone ssh://gitredgreen/RedGreen/bzdoor.git")
  message("*** (Or whatever git clone you need for door++)")
  message("***")
endif()

add_subdirectory(door++)

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

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/SQLiteCpp)
  message("***")
  message("*** ERROR/MISSING *** please run: git clone https://github.com/SRombauts/SQLiteCpp.git")
  message("***")
endif()

set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
add_subdirectory(SQLiteCpp)

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp)
  message("***")
  message("*** ERROR/MISSING *** please run: git clone https://github.com/jbeder/yaml-cpp.git")
  message("***")
endif()

add_subdirectory(yaml-cpp)

add_executable(space-ace main.cpp deck.cpp db.h db.cpp images.h)
target_link_libraries(space-ace door++ pthread SQLiteCpp sqlite3 dl)

