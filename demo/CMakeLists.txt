cmake_minimum_required(VERSION 3.15...3.27)
project(zombie-demo)

set(ZOMBIE_DEMO_SRC_FILES
    "config.h"
    "colormap.h"
    "image.h"
    "model_problem.h"
    "grid.h"
)

# add dependencies
list(APPEND ZOMBIE_DEMO_DEPS_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/deps/stb)
list(APPEND ZOMBIE_DEMO_DEPS_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/deps/nlohmann)

# build demo
add_executable(demo demo.cpp ${ZOMBIE_DEMO_SRC_FILES})
target_include_directories(demo PRIVATE ${ZOMBIE_DEMO_DEPS_INCLUDES})
target_link_libraries(demo zombie)

if(MSVC)
    target_compile_options(demo PRIVATE /bigobj)
endif()
