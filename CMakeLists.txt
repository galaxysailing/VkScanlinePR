cmake_minimum_required(VERSION 3.0.0)
project(Playground VERSION 0.1.0)

## set CPACK_PROJECT_NAME
if(DEFINED ENV{CMAKE_PROJECT_NAME})
  message("Use environment project name: " $ENV{CMAKE_PROJECT_NAME})
  set(CPACK_PROJECT_NAME $ENV{CMAKE_PROJECT_NAME})
else()
  set(CPACK_PROJECT_NAME ${PROJECT_NAME})
  set(ENV{CMAKE_PROJECT_NAME} ${CPACK_PROJECT_NAME})
endif()

## set CPACK_PROJECT_VERSION
if(DEFINED ENV{CMAKE_PROJECT_VERSION})
  message("Use environment project version: " $ENV{CMAKE_PROJECT_NAME})
  set(CPACK_PROJECT_VERSION $ENV{CMAKE_PROJECT_VERSION})
else()
  set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
  set(ENV{CMAKE_PROJECT_VERSION} ${CPACK_PROJECT_VERSION})
endif()

## set Compiler Flags.
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
CHECK_CXX_COMPILER_FLAG("-std=c++1z" COMPILER_SUPPORTS_CXX1Z)
if(COMPILER_SUPPORTS_CXX17)
  message("Use std C++17.")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
elseif(COMPILER_SUPPORTS_CXX1Z)
  message("Use std C++1z.")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
else()
  message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++17 support. Please use a different C++ compiler.")
endif()

## extra compiler warnings
if ("${CMAKE_C_COMPILER_ID}" MATCHES ".*Clang.*")
	SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wunused-variable -Werror")
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
	SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wunused-but-set-variable -Wunused-variable -Werror")
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "Intel")
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/linux_pc)

set(SHADERC_SKIP_TESTS ON CACHE BOOL "Shaderc tests" FORCE)
set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_COPYRIGHT_CHECK ON CACHE BOOL "" FORCE)
set(SPIRV-Headers_SOURCE_DIR "3rdparty" CACHE BOOL "" FORCE)

## support pkg_check_modules
find_package(PkgConfig REQUIRED)

add_subdirectory(3rdparty/glfw)
add_subdirectory(3rdparty/glm)
add_subdirectory(3rdparty/SPIRV-Headers)
add_subdirectory(3rdparty/spirv-tools)
add_subdirectory(3rdparty/glslang)
add_subdirectory(3rdparty/shaderc)

pkg_check_modules(LIBUSB REQUIRED vulkan)
include_directories(${VULKAN_INCLUDE_DIRS})
link_directories(${VULKAN_LIBRARY_DIRS})
link_libraries(${VULKAN_LIBRARIES})

add_subdirectory(src)
# add_subdirectory(test)