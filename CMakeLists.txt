project(codal-game-engine)

include("${CODAL_UTILS_LOCATION}")

# find sources and headers
RECURSIVE_FIND_DIR(TOP_LEVEL_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/inc" "*.h")

RECURSIVE_FIND_FILE(TOP_LEVEL_SOURCE0 "${CMAKE_CURRENT_SOURCE_DIR}/source" "*.cpp")
RECURSIVE_FIND_FILE(TOP_LEVEL_SOURCE1 "${CMAKE_CURRENT_SOURCE_DIR}/source" "*.c")

# add them
include_directories(${TOP_LEVEL_INCLUDE_DIRS})

# create our target
add_library(codal-game-engine ${TOP_LEVEL_SOURCE0} ${TOP_LEVEL_SOURCE1})

target_link_libraries(codal-game-engine codal-core)

# expose it to parent cmake.
target_include_directories(codal-game-engine PUBLIC ${TOP_LEVEL_INCLUDE_DIRS})
