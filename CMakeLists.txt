cmake_minimum_required(VERSION 3.11)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("textRPG")

FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG main
)
FetchContent_MakeAvailable(SDL3)

set(SDLIMAGE_AVIF OFF CACHE BOOL "Disable AVIF" FORCE)
set(SDLIMAGE_AVIF_SHARED OFF CACHE BOOL "Disable AVIF Shared" FORCE)
set(SDLIMAGE_AVIF_SAVE OFF CACHE BOOL "Disable AVIF Save" FORCE)

FetchContent_Declare(
        SDL3_image
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
        GIT_TAG main
)
FetchContent_MakeAvailable(SDL3_image)

FetchContent_Declare(
        SDL3_ttf
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
        GIT_TAG main
)
FetchContent_MakeAvailable(SDL3_ttf)

FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)

file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

# Filter out main.cpp and main_cli.cpp for the shared engine files
set(ENGINE_SOURCES ${ALL_SOURCES})
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/main(_cli)?\\.cpp$")

# Target 1: GUI Executable (textRPG)
add_executable(textRPG ${ENGINE_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
target_include_directories(textRPG PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(textRPG PRIVATE
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_ttf::SDL3_ttf
        nlohmann_json::nlohmann_json
)

# Target 2: Headless CLI Executable (textRPG_cli)
add_executable(textRPG_cli ${ENGINE_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/src/main_cli.cpp")
target_include_directories(textRPG_cli PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(textRPG_cli PRIVATE
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_ttf::SDL3_ttf
        nlohmann_json::nlohmann_json
)

# Post-Build Data Directory Copy
add_custom_command(TARGET textRPG POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/data"
        "$<TARGET_FILE_DIR:textRPG>/data"
)

add_custom_command(TARGET textRPG_cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/data"
        "$<TARGET_FILE_DIR:textRPG_cli>/data"
)

if(WIN32)
  add_custom_command(TARGET textRPG POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "$<TARGET_FILE:SDL3::SDL3>"
          "$<TARGET_FILE_DIR:textRPG>"
  )
  add_custom_command(TARGET textRPG POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "$<TARGET_FILE:SDL3_ttf::SDL3_ttf>"
          "$<TARGET_FILE_DIR:textRPG>"
  )
endif()