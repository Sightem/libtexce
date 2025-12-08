cmake_minimum_required(VERSION 3.20)

project(libtexce C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output executables to bin/ for easy access
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(ENABLE_HOST_TESTS      "Enable host tests/tools" ON)
option(ENABLE_STRICT_WARNINGS "Enable aggressive warnings and treat them as errors" ON)
option(ENABLE_CLANG_TIDY      "Run clang-tidy during builds when available" ON)

if(ENABLE_STRICT_WARNINGS)
  set(_STRICT_COMMON_WARNINGS
    -Wall -Wextra -Wpedantic -Werror
    -Wconversion -Wsign-conversion
    -Wshadow -Wdouble-promotion
    -Wformat=2 -Wformat-security
    -Wcast-qual -Wpointer-arith
    -Wswitch-enum -Wundef
    -Wnull-dereference -Wfloat-equal
    -Wredundant-decls -Wunreachable-code
    -Wimplicit-fallthrough
  )
  add_compile_options(${_STRICT_COMMON_WARNINGS})
  # C-specific warnings
  add_compile_options("$<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes>")
  add_compile_options("$<$<COMPILE_LANGUAGE:C>:-Wmissing-prototypes>")
  add_compile_options("$<$<COMPILE_LANGUAGE:C>:-Wold-style-definition>")
  # C++-specific warnings
  add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>")
  add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>")
endif()

if(ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(_clang_tidy_cmd ${CLANG_TIDY_EXE} "-warnings-as-errors=*")
    set(CMAKE_C_CLANG_TIDY   ${_clang_tidy_cmd})
    set(CMAKE_CXX_CLANG_TIDY ${_clang_tidy_cmd})
  else()
    message(WARNING "ENABLE_CLANG_TIDY=ON but clang-tidy executable was not found")
  endif()
endif()

# ---------------------------------------------------------------------------
# Core engine as an OBJECT library
# ---------------------------------------------------------------------------
set(TEX_CORE_SOURCES
  src/tex/tex_util.c
  src/tex/tex_arena.c
  src/tex/tex_symbols.c
  src/tex/tex_metrics.c
  src/tex/tex_fonts.c
  src/tex/tex_token.c
  src/tex/tex_parse.c
  src/tex/tex_measure.c
  src/tex/tex_layout.c
  src/tex/tex_renderer.c
  src/tex/tex_draw.c
)

add_library(tex_core OBJECT ${TEX_CORE_SOURCES})
target_include_directories(tex_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tex
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ---------------------------------------------------------------------------
# Host tests (native)
# ---------------------------------------------------------------------------
if(ENABLE_HOST_TESTS AND NOT CMAKE_CROSSCOMPILING)
  add_executable(test_token tests/test_token.c $<TARGET_OBJECTS:tex_core>)
  add_executable(test_parse tests/test_parse.c $<TARGET_OBJECTS:tex_core>)
  add_executable(test_measure tests/test_measure.c $<TARGET_OBJECTS:tex_core>)
  add_executable(test_layout tests/test_layout.c $<TARGET_OBJECTS:tex_core>)
  add_executable(test_draw tests/test_draw.c $<TARGET_OBJECTS:tex_core>)
  add_executable(test_arena tests/test_arena.c $<TARGET_OBJECTS:tex_core>)
  add_executable(test_symbols tests/test_symbols.c $<TARGET_OBJECTS:tex_core>)
  add_executable(test_errors tests/test_errors.c $<TARGET_OBJECTS:tex_core>)

  set(_TEX_INC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tex
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  foreach(tgt IN ITEMS test_token test_parse test_measure test_layout test_draw test_arena test_symbols test_errors)
    target_include_directories(${tgt} PRIVATE ${_TEX_INC})
  endforeach()

  add_custom_target(run_tests
    COMMAND $<TARGET_FILE:test_token>
    COMMAND $<TARGET_FILE:test_parse>
    COMMAND $<TARGET_FILE:test_measure>
    COMMAND $<TARGET_FILE:test_layout>
    COMMAND $<TARGET_FILE:test_draw>
    COMMAND $<TARGET_FILE:test_arena>
    COMMAND $<TARGET_FILE:test_symbols>
    COMMAND $<TARGET_FILE:test_errors>
    DEPENDS test_token test_parse test_measure test_layout test_draw test_arena test_symbols test_errors
  )
endif()
