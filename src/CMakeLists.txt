# Include .h ###################################################################
include_directories(.)
include_directories(./app)
include_directories(./core)
include_directories(./core/common)
include_directories(./core/scanline)
include_directories(./core/vg)
include_directories(./core/vk)
include_directories(./core/vulkan)

# find library #################################################################
# pkg_check_modules(GLIB REQUIRED glfw)
# include_directories(${GLFW_INCLUDE_DIRS})
# link_directories(${GLFW_LIBRARY_DIRS})

# pkg_check_modules(GLIB REQUIRED vulkan)
# include_directories(${VULKAN_INCLUDE_DIRS})
# link_directories(${VULKAN_LIBRARY_DIRS})

# pkg_check_modules(GLIB REQUIRED glm)
include_directories("../3rdparty/glm/")
# link_directories(${glm_LIBRARY_DIRS})

# include .c/.cpp ##############################################################
file (GLOB SOURCE_CPP
  "./app/*.cpp"
  "./core/*.cpp"
  "./core/common/*.cpp"
  "./core/scanline/*.cpp"
  "./core/vg/*.cpp"
  "./core/vk/*.cpp"
  "./core/vulkan/*.cpp"
)

# generate exec obj ############################################################
add_executable( ${CPACK_PROJECT_NAME}
  "main.cpp"
  ${SOURCE_CPP}
)

# link librarys ################################################################
target_link_libraries(${CPACK_PROJECT_NAME} pthread)
target_link_libraries(${CPACK_PROJECT_NAME} glfw)
target_link_libraries(${CPACK_PROJECT_NAME} vulkan)
target_link_libraries(${CPACK_PROJECT_NAME} shaderc)
# target_link_libraries(${CPACK_PROJECT_NAME} glm)
