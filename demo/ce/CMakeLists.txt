cmake_minimum_required(VERSION 3.20)

# Self-contained CE demos (copy-paste friendly)
# - Builds two independent programs: demo_text and demo_thread
# - Uses CEdevToolchain.cmake directly (no dependency on root CMake)

project(libtexce_ce_demos C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DEMO_BUILD_AS_CXX "Compile demo sources with the C++ compiler to check compatibility" OFF)

# Bring in CEdev toolchain helper and cedev_add_program()
# Adjust CEDEV_ROOT via -DCEDEV_ROOT=/path if needed.
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/CEdevToolchain.cmake)

# Path to repo root (for engine sources/headers)
get_filename_component(TEX_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

# Core engine sources (copied 1:1 from root list)
set(TEX_CORE_SOURCES
  ${TEX_ROOT}/src/tex/tex_util.c
  ${TEX_ROOT}/src/tex/tex_arena.c
  ${TEX_ROOT}/src/tex/tex_symbols.c
  ${TEX_ROOT}/src/tex/tex_metrics.c
  ${TEX_ROOT}/src/tex/tex_fonts.c
  ${TEX_ROOT}/src/tex/tex_token.c
  ${TEX_ROOT}/src/tex/tex_parse.c
  ${TEX_ROOT}/src/tex/tex_measure.c
  ${TEX_ROOT}/src/tex/tex_layout.c
  ${TEX_ROOT}/src/tex/tex_renderer.c
  ${TEX_ROOT}/src/tex/tex_draw.c
)

set(TEX_INCLUDE_DIRS
  ${TEX_ROOT}/src
  ${TEX_ROOT}/src/tex
  ${TEX_ROOT}/include
)

set(TEX_COMPILE_DEFS
  -DTEX_USE_FONTLIB
  -DTEX_DIRECT_RENDER
)

# -----------------------------------------------------------------------------
# Demo: Scrollable text renderer
# -----------------------------------------------------------------------------
set(_DEMO_LANG_ARG)
if(DEMO_BUILD_AS_CXX)
  list(APPEND _DEMO_LANG_ARG CXX_MODE)
endif()

cedev_add_program(
  TARGET demo_text
  NAME   "DTEXT"
  DESCRIPTION "CE demo: TeX renderer (scrollable text)"

  SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/demo_text.cpp
    ${TEX_CORE_SOURCES}

  INCLUDE_DIRECTORIES
    ${TEX_INCLUDE_DIRS}

  LIBLOAD
    graphx keypadc fontlibc

  ${_DEMO_LANG_ARG}

  COMPILE_OPTIONS
    ${TEX_COMPILE_DEFS}

  LINKER_GLOBALS
    "range .bss $D28000 : $D37FFE"
    "provide __stack = $D37FFF"
    "locate .header at $D1A87F"
)

# -----------------------------------------------------------------------------
# Demo: Threaded chat-style conversation renderer
# -----------------------------------------------------------------------------
cedev_add_program(
  TARGET demo_thread
  NAME   "DTHREAD"
  DESCRIPTION "CE demo: TeX renderer (chat thread)"

  SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/demo_thread.c
    ${TEX_CORE_SOURCES}

  INCLUDE_DIRECTORIES
    ${TEX_INCLUDE_DIRS}

  LIBLOAD
    graphx keypadc fontlibc

  ${_DEMO_LANG_ARG}

  COMPILE_OPTIONS
    ${TEX_COMPILE_DEFS}

  LINKER_GLOBALS
    "range .bss $D28000 : $D37FFE"
    "provide __stack = $D37FFF"
    "locate .header at $D1A87F"
)
