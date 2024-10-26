cmake_minimum_required(VERSION 3.4...3.28 FATAL_ERROR)

if(NOT WIN32)
    message(FATAL_ERROR "Currently, only Windows is supported")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


project(vulkan_sandbox LANGUAGES CXX)


set(AM_PROJECT_SOURCE_DIR                   ${CMAKE_CURRENT_LIST_DIR})
set(AM_PROJECT_CXX_SOURCE_CODE_DIR          ${AM_PROJECT_SOURCE_DIR}/source)
set(AM_PROJECT_SHADERS_SOURCE_CODE_DIR      ${AM_PROJECT_CXX_SOURCE_CODE_DIR}/shaders)
set(AM_PROJECT_BINARY_OUTPUT_DIR            ${AM_PROJECT_SOURCE_DIR}/binary)
set(AM_PROJECT_CONFIG_DIR                   ${AM_PROJECT_SOURCE_DIR}/config)
set(AM_PROJECT_THIRD_PARTY_DIR              ${AM_PROJECT_SOURCE_DIR}/thirdparty)

set(AM_PROJECT_THIRDPARTY_GLFW_DIR          ${AM_PROJECT_THIRD_PARTY_DIR}/glfw)
set(AM_PROJECT_THIRDPARTY_GLM_DIR           ${AM_PROJECT_THIRD_PARTY_DIR}/glm)
set(AM_PROJECT_THIRDPARTY_NLOHMAN_JSON_DIR  ${AM_PROJECT_THIRD_PARTY_DIR}/nlohmann_json)
set(AM_PROJECT_THIRDPARTY_SPDLOG_DIR        ${AM_PROJECT_THIRD_PARTY_DIR}/spdlog)

message("-- PROJECT_SOURCE_DIR                  = ${AM_PROJECT_SOURCE_DIR}")
message("-- PROJECT_CXX_SOURCE_CODE_DIR         = ${AM_PROJECT_CXX_SOURCE_CODE_DIR}")
message("-- PROJECT_SHADERS_SOURCE_CODE_DIR     = ${AM_PROJECT_SHADERS_SOURCE_CODE_DIR}")
message("-- PROJECT_BINARY_OUTPUT_DIR           = ${AM_PROJECT_BINARY_OUTPUT_DIR}")
message("-- PROJECT_CONFIG_DIR                  = ${AM_PROJECT_CONFIG_DIR}")
message("-- PROJECT_THIRD_PARTY_DIR             = ${AM_PROJECT_THIRD_PARTY_DIR}")
message("")
message("-- PROJECT_THIRDPARTY_GLFW_DIR         = ${AM_PROJECT_THIRDPARTY_GLFW_DIR}")
message("-- PROJECT_THIRDPARTY_GLM_DIR          = ${AM_PROJECT_THIRDPARTY_GLM_DIR}")
message("-- PROJECT_THIRDPARTY_NLOHMAN_JSON_DIR = ${AM_PROJECT_THIRDPARTY_NLOHMAN_JSON_DIR}")
message("-- PROJECT_THIRDPARTY_SPDLOG_DIR       = ${AM_PROJECT_THIRDPARTY_SPDLOG_DIR}")
message("")
message("-- PROJECT_BUILD_DIR                   = ${CMAKE_BINARY_DIR}")
message("")


add_subdirectory(${AM_PROJECT_THIRDPARTY_GLFW_DIR})


set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(${AM_PROJECT_THIRDPARTY_NLOHMAN_JSON_DIR})


add_subdirectory(${AM_PROJECT_THIRDPARTY_SPDLOG_DIR})


file(GLOB_RECURSE PROJECT_SOURCE_FILES CONFIGURE_DEPENDS 
    ${AM_PROJECT_CXX_SOURCE_CODE_DIR}/*.cpp
    ${AM_PROJECT_CXX_SOURCE_CODE_DIR}/*.c
)
add_executable(vulkan_sandbox ${PROJECT_SOURCE_FILES})


target_precompile_headers(vulkan_sandbox PRIVATE ${AM_PROJECT_CXX_SOURCE_CODE_DIR}/pch.h)


set(SHADERC_ENABLE_SHARED_CRT ON)

find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

if(Vulkan_FOUND)
    message("-- Vulkan_VERSION      = ${Vulkan_VERSION}")
    message("-- Vulkan_INCLUDE_DIRS = ${Vulkan_INCLUDE_DIRS}")
    message("-- Vulkan_LIBRARIES    = ${Vulkan_LIBRARIES}")

    set(AM_GRAPHICS_API AM_GRAPHICS_API_VULKAN)
else()
    message(FATAL_ERROR "Vulkan not found!")
endif()


target_include_directories(vulkan_sandbox 
    PRIVATE ${AM_PROJECT_CXX_SOURCE_CODE_DIR}

    PRIVATE ${Vulkan_INCLUDE_DIRS}
    PRIVATE ${Python_INCLUDE_DIRS}
    PRIVATE ${GLFW_INCLUDE_DIRS}
    PRIVATE ${AM_PROJECT_THIRDPARTY_GLM_DIR}
    PRIVATE ${nlohmann_json_INCLUDE_DIRS}
    PRIVATE ${spdlog_INCLUDE_DIRS})


target_link_directories(vulkan_sandbox
    PRIVATE ${spdlog_LIBRARY_DIRS})

target_link_libraries(vulkan_sandbox 
    PRIVATE Vulkan::Vulkan
    PRIVATE Vulkan::shaderc_combined
    PRIVATE glfw 
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE spdlog::spdlog)

target_compile_definitions(vulkan_sandbox 
    PRIVATE AM_PROJECT_SOURCE_DIR="${AM_PROJECT_SOURCE_DIR}"
    PRIVATE AM_PROJECT_CXX_SOURCE_CODE_DIR="${AM_PROJECT_CXX_SOURCE_CODE_DIR}"
    PRIVATE AM_PROJECT_SHADERS_SOURCE_CODE_DIR="${AM_PROJECT_SHADERS_SOURCE_CODE_DIR}"
    PRIVATE AM_PROJECT_BINARY_OUTPUT_DIR="${AM_PROJECT_BINARY_OUTPUT_DIR}"
    PRIVATE AM_PROJECT_CONFIG_DIR="${AM_PROJECT_CONFIG_DIR}"
    PRIVATE SPDLOG_COMPILED_LIB
    
    PRIVATE ${AM_GRAPHICS_API})