cmake_minimum_required(VERSION 3.15)

project(detectionx LANGUAGES CXX)

# ============================================================
# Global build settings
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

# Runtime paths (for dynamic libraries)
set(CMAKE_BUILD_RPATH "$ORIGIN/lib")
set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")

# Output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bins)

# ============================================================
# Runtime backend selection (RKNN / TRT / ORT)
# ============================================================
set(RUNTIME "rknn" CACHE STRING "Select inference backend")
set_property(CACHE RUNTIME PROPERTY STRINGS rknn trt ort)

message(STATUS "Selected backend: ${RUNTIME}")

# ============================================================
# Third-party / dependency management
# ============================================================
include(${PROJECT_SOURCE_DIR}/cmake/dependence.cmake)

# ============================================================
# Executable
# ============================================================
include_directories(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/include ${X3RDPARTY_INCLUDE_DIRS})

file(GLOB SRCS src/*.cpp)
add_executable(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME} pthread ${X3RDPARTY_LINK_LIBS})

# ============================================================
# Release
# ============================================================
set(RELEASE_DIR ${PROJECT_SOURCE_DIR}/release)
install(FILES ${PROJECT_SOURCE_DIR}/config.yaml DESTINATION ${RELEASE_DIR})
install(DIRECTORY ${PROJECT_SOURCE_DIR}/bins/ DESTINATION ${RELEASE_DIR} USE_SOURCE_PERMISSIONS)
#install(DIRECTORY ${PROJECT_SOURCE_DIR}/asset/images DESTINATION ${RELEASE_DIR})
#install(DIRECTORY ${PROJECT_SOURCE_DIR}/asset/models DESTINATION ${RELEASE_DIR})

install(FILES ${X3RDPARTY_INSTALL_FILES} DESTINATION ${RELEASE_DIR}/lib)
