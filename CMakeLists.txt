# CMakeList.txt : CMake project for textRPG, include source and define
# project specific logic here.

cmake_minimum_required (VERSION 3.11)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("textRPG")

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

# Add source to this project's executable.
add_executable (textRPG 
    "src/main.cpp" 
    "src/game.h" 
    "src/game.cpp"
    "src/gameMap.h"
    "src/gameMap.cpp"
    "src/entity.h"
    "src/entity.cpp"
    "src/enchantment.h"
    "src/item.h"
    "src/inventory.h"
    "src/inventory.cpp"
    "src/enums.h"
    "src/itemDatabase.h"
    "src/itemDatabase.cpp"
 "src/actionButton.h")

# Link the libraries to the executable
target_link_libraries(textRPG PRIVATE 
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
    nlohmann_json::nlohmann_json
)

# Copy SDL3.dll to the build directory automatically
add_custom_command(TARGET textRPG POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/_deps/sdl3-build/sdl3.dll" 
    "$<TARGET_FILE_DIR:textRPG>"
)

add_custom_command(TARGET textRPG POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/data"
    "$<TARGET_FILE_DIR:textRPG>/data"
)