# PTN101 Top Level CMakeLists.txt

# To find the cmake version run
# $ cmake --version
cmake_minimum_required( VERSION 3.15 FATAL_ERROR )

set(PTN101_TOP_DIR
  "${CMAKE_SOURCE_DIR}"
)

# Set sub cmake module file paths
set(CMAKE_MODULE_PATH
  "${PTN101_TOP_DIR}/CMake"
  "${CMAKE_CURRENT_BINARY_DIR}/../"
  ${CMAKE_MODULE_PATH}
)

# Use ccache if it is available.
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  message(STATUS "CCACHE FOUND = ${CCACHE_PROGRAM}")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()  

# Include sub cmake module files
include(compiler_options)
include(cross_compilation)
include(search_functions)
include(include_path)
include(project_description)

# Set the project. ${THE_TARGET} resides in project_description.cmake
project ( ${THE_TARGET} C ASM )

# PTN101 only supports Debug build
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug'")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

# Clear CMake extra compiler flags for Debug build. We have manually set the flags in compiler_options.cmake
set(CMAKE_ASM_FLAGS_DEBUG "" CACHE STRING "Set Debug build assembler pre-set flags to none." FORCE)
set(CMAKE_C_FLAGS_DEBUG "" CACHE STRING "Set Debug build compiler pre-set flags to none" FORCE)

# We currently build the project by finding all sources
# and header files.  We may then exclude specific files below.
#
# In the future it would perhaps be nicer to explicitly list the files we want
# to include, and to move the source lists in to separate per-subdirectory
# CMakeLists.txt files.
# ${PTN101_SRC_DIR} locates at project_description.cmake 
foreach(PATH ${PTN101_SRC_DIR})
  auto_sources(_files "*.c" "RECURSE" ${PATH})
  auto_sources(_sfiles "*.S" "RECURSE" ${PATH})
  auto_sources(_hfiles "*.h" "RECURSE" ${PATH})  
  list(APPEND src_files ${_files})
  list(APPEND asm_files ${_sfiles})
  list(APPEND header_files ${_hfiles})
endforeach()

if(EXCLUDE_DIR)
  REMOVE_MATCHES_FROM_LISTS(src_files asm_files
    MATCHES
    ${EXCLUDE_DIR}
  )
endif()

# Add source group for IDE
auto_source_group(PTN101 ${CMAKE_CURRENT_SOURCE_DIR} ${src_files} ${header_files} ${asm_files} )

# PTN101 only supports Debug build. Check the build type before compiling.
if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(FATAL_ERROR "PTN101 only supports Debug build.")
endif()

if(BUILD_AS_LIB AND BUILD_AS_LIB MATCHES "yes")
  add_library( ${PROJECT_NAME} STATIC ${src_files} ${header_files} )
else ()
# Add the executable
  add_executable( ${PROJECT_NAME} ${src_files} ${asm_files} ${header_files} )

# Link libraries
  target_link_libraries(${PROJECT_NAME} 
    PRIVATE
        m
        ${LIBS}
  )  
  # Set the postfix of the output executable file.
  set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ".elf")
endif()

# Target Include Path
target_include_directories(${PROJECT_NAME}
    PRIVATE 
        ${INC_PATH}
)

# Apply compiler and link options
apply_compile_options_to_target(${PROJECT_NAME})
apply_link_options_to_target(${PROJECT_NAME})
