# The Vita embedder, for an app build to add with add_subdirectory().
#
# An OBJECT library rather than an archive: main() and the weak hooks live in
# vita_embedder.cc, and objects reach the link line whether or not anything
# references them by symbol. The app links the target and gets the objects,
# the include/ directory, and nothing else.
#
# The caller sets FLUTTER_VITA_ENGINE_INCLUDE to the engine artifact's
# include/ (embedder.h, embedder_software_frame_damage.h, engine_version.h)
# before adding this directory. The compiler flags are inherited from the
# caller; the flutter-vita template builds every file with -std=gnu++20 -O2.
#
# The dev server (src/devserver.c) is not part of the target. It serves the
# edit-deploy-run loop and a release build links stubs in its place, so the
# app build adds it, or not, beside its other sources.
cmake_minimum_required(VERSION 3.16)
project(flutter_vita_embedder C CXX)

if(NOT FLUTTER_VITA_ENGINE_INCLUDE)
  message(FATAL_ERROR
    "Set FLUTTER_VITA_ENGINE_INCLUDE to the engine artifact's include/ "
    "(flutter-engine-vita-<version>/include) before add_subdirectory().")
endif()

add_library(flutter_vita_embedder OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/src/vita_embedder.cc)
target_include_directories(flutter_vita_embedder
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${FLUTTER_VITA_ENGINE_INCLUDE})
# The engine archive is built with -fno-strict-aliasing; the embedder shares
# structs with it and is built the same way. -Wno-psabi silences GCC's note
# about the ARM ABI change for std::function parameters.
target_compile_options(flutter_vita_embedder PRIVATE -fno-strict-aliasing -Wno-psabi)

set(FLUTTER_VITA_DEVSERVER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/devserver.c
    CACHE FILEPATH "the dev server, for a debug build to compile beside the app")
