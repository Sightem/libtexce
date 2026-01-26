cmake_minimum_required(VERSION 3.20)

project(libtexce_matrix_ce C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CEDEV_HAS_PRINTF OFF CACHE BOOL "Link toolchain printf implementations" FORCE)
include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/CEdevToolchain.cmake)

get_filename_component(TEX_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

set(TEX_CORE_SOURCES
  ${TEX_ROOT}/src/tex/tex_util.c
  ${TEX_ROOT}/src/tex/tex_pool.c
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

set(MATRIX_APP_SOURCES
  ${TEX_ROOT}/apps/matrix/src/main.cpp
  ${TEX_ROOT}/apps/matrix/src/app.cpp
  ${TEX_ROOT}/apps/matrix/src/app_draw.cpp
  ${TEX_ROOT}/apps/matrix/src/input.cpp
  ${TEX_ROOT}/apps/matrix/src/rational.cpp
  ${TEX_ROOT}/apps/matrix/src/matrix_ops.cpp
  ${TEX_ROOT}/apps/matrix/src/steps.cpp
  ${TEX_ROOT}/apps/matrix/src/ui_menu.cpp
  ${TEX_ROOT}/apps/matrix/src/screens_main_menu.cpp
  ${TEX_ROOT}/apps/matrix/src/screens_matrices_menu.cpp
  ${TEX_ROOT}/apps/matrix/src/screens_operations_menu.cpp
  ${TEX_ROOT}/apps/matrix/src/screens_slot_picker.cpp
  ${TEX_ROOT}/apps/matrix/src/screens_result_view.cpp
  ${TEX_ROOT}/apps/matrix/src/screens_steps_menu.cpp
  ${TEX_ROOT}/apps/matrix/src/screens_steps_view.cpp
  ${TEX_ROOT}/apps/matrix/src/screens_matrix_resize.cpp
  ${TEX_ROOT}/apps/matrix/src/screens_matrix_editor.cpp
  ${TEX_ROOT}/apps/matrix/src/screens_element_picker.cpp
  ${TEX_ROOT}/apps/matrix/src/screens_element_output.cpp
  ${TEX_ROOT}/apps/matrix/src/screens_stub.cpp
  ${TEX_ROOT}/apps/matrix/src/screens_dispatch.cpp
)

cedev_add_program(
  TARGET matrix
  NAME   "MATRIX"
  DESCRIPTION "Matrix tools (libtexce)"

  SOURCES
    ${MATRIX_APP_SOURCES}
    ${TEX_CORE_SOURCES}

  INCLUDE_DIRECTORIES
    ${TEX_INCLUDE_DIRS}
    ${TEX_ROOT}/apps/matrix/src

  LIBLOAD
    graphx fontlibc keypadc

  COMPILE_OPTIONS
    ${TEX_COMPILE_DEFS}
)
