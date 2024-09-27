cmake_minimum_required(VERSION 3.4...3.28 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(vulkan_sandbox LANGUAGES CXX)

set(AM_PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR})
set(THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/thirdparty)
set(CODE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/source)

set(GLFW_DIR ${THIRD_PARTY_DIR}/glfw)
set(GLM_DIR ${THIRD_PARTY_DIR}/glm)
set(NLOHMAN_JSON_DIR ${THIRD_PARTY_DIR}/nlohmann_json)

message("-- PROJECT_SOURCE_DIR = ${AM_PROJECT_SOURCE_DIR}")
message("-- THIRD_PARTY_DIR    = ${THIRD_PARTY_DIR}")
message("-- CODE_SOURCE_DIR    = ${CODE_SOURCE_DIR}")
message("-- GLFW_DIR           = ${GLFW_DIR}")
message("-- GLM_DIR            = ${GLM_DIR}")
message("-- NLOHMAN_JSON_DIR   = ${NLOHMAN_JSON_DIR}")

add_subdirectory(${GLFW_DIR})

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(${NLOHMAN_JSON_DIR})


file(GLOB_RECURSE SOURCE CONFIGURE_DEPENDS 
    ${CODE_SOURCE_DIR}/*.cpp
    ${CODE_SOURCE_DIR}/*.c
)
add_executable(vulkan_sandbox ${SOURCE})

target_precompile_headers(vulkan_sandbox PRIVATE ${CODE_SOURCE_DIR}/pch.h)

find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    message("-- Vulkan_VERSION      = ${Vulkan_VERSION}")
    message("-- Vulkan_INCLUDE_DIRS = ${Vulkan_INCLUDE_DIRS}")
    message("-- Vulkan_LIBRARIES    = ${Vulkan_LIBRARIES}")
else()
    message(FATAL_ERROR "Vulkan not found!")
endif()

target_include_directories(vulkan_sandbox 
    PRIVATE ${GLM_DIR} 
    PRIVATE ${Vulkan_INCLUDE_DIRS})

target_link_libraries(vulkan_sandbox 
    PRIVATE glfw 
    PRIVATE ${Vulkan_LIBRARIES} 
    PRIVATE nlohmann_json::nlohmann_json)

target_compile_definitions(vulkan_sandbox PRIVATE AM_PROJECT_SOURCE_DIR="${AM_PROJECT_SOURCE_DIR}")