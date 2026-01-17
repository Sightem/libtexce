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
    set(_clang_tidy_cmd ${CLANG_TIDY_EXE}
      "-header-filter=${CMAKE_SOURCE_DIR}/src/.*"
      "-checks=-clang-analyzer-optin.core.EnumCastOutOfRange,-bugprone-narrowing-conversions,-bugprone-easily-swappable-parameters,-clang-analyzer-security.insecureAPI.strcpy,-clang-diagnostic-missing-field-initializers"
      "-warnings-as-errors=*"
    )
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
  src/tex/tex_pool.c
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

option(ENABLE_PORTCE_DEMO "Build demo_text using PortCE for native SDL rendering" ON)
if(ENABLE_PORTCE_DEMO AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/portce)
  set(_SAVED_C_CLANG_TIDY "${CMAKE_C_CLANG_TIDY}")
  set(_SAVED_CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}")
  set(CMAKE_C_CLANG_TIDY "")
  set(CMAKE_CXX_CLANG_TIDY "")

  add_subdirectory(portce)

  set(CMAKE_C_CLANG_TIDY "${_SAVED_C_CLANG_TIDY}")
  set(CMAKE_CXX_CLANG_TIDY "${_SAVED_CXX_CLANG_TIDY}")
  
  target_compile_options(PortCE PRIVATE
    -Wno-error
    -Wno-bit-int-extension
    -Wno-old-style-cast
  )
  target_include_directories(tex_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/portce/src/include
  )
  target_compile_options(tex_core PRIVATE
    -Wno-bit-int-extension
    -Wno-implicit-int-conversion
    -Wno-strict-prototypes
  )
endif()

# ---------------------------------------------------------------------------
# Host tests (native)
# ---------------------------------------------------------------------------
if(ENABLE_HOST_TESTS AND NOT CMAKE_CROSSCOMPILING)
  find_package(SDL2 QUIET)

  if(ENABLE_PORTCE_DEMO AND SDL2_FOUND AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/portce)
    add_executable(test_token tests/test_token.c $<TARGET_OBJECTS:tex_core>)
    add_executable(test_parse tests/test_parse.c $<TARGET_OBJECTS:tex_core>)
    add_executable(test_measure tests/test_measure.c $<TARGET_OBJECTS:tex_core>)
    add_executable(test_layout tests/test_layout.c $<TARGET_OBJECTS:tex_core>)
    # add_executable(test_draw tests/test_draw.c $<TARGET_OBJECTS:tex_core>)
    add_executable(test_symbols tests/test_symbols.c $<TARGET_OBJECTS:tex_core>)
    # add_executable(test_errors tests/test_errors.c $<TARGET_OBJECTS:tex_core>)
    add_executable(test_pool tests/test_pool.c $<TARGET_OBJECTS:tex_core>)

    # Copy font appvars to build directory for tests
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/appvar)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets)
      file(GLOB FONT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/*.8xv")
      foreach(font_file ${FONT_FILES})
        get_filename_component(font_name ${font_file} NAME)
        configure_file(${font_file} ${CMAKE_BINARY_DIR}/appvar/${font_name} COPYONLY)
      endforeach()
    endif()

    set(_TEX_INC
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/src/tex
      ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    foreach(tgt IN ITEMS test_token test_parse test_measure test_layout test_symbols test_pool)
      target_include_directories(${tgt} PRIVATE ${_TEX_INC})
      target_link_libraries(${tgt} PRIVATE PortCE -lm)
    endforeach()

    # Register tests with CTest
    enable_testing()
    add_test(NAME token   COMMAND test_token)
    add_test(NAME parse   COMMAND test_parse)
    add_test(NAME measure COMMAND test_measure)
    add_test(NAME layout  COMMAND test_layout)
    # add_test(NAME draw    COMMAND test_draw)  # disabled
    add_test(NAME symbols COMMAND test_symbols)
    # add_test(NAME errors  COMMAND test_errors)  # disabled
    add_test(NAME pool    COMMAND test_pool)

    add_custom_target(run_tests
      COMMAND $<TARGET_FILE:test_token>
      COMMAND $<TARGET_FILE:test_parse>
      COMMAND $<TARGET_FILE:test_measure>
      COMMAND $<TARGET_FILE:test_layout>
      # COMMAND $<TARGET_FILE:test_draw>  # disabled
      COMMAND $<TARGET_FILE:test_symbols>
      # COMMAND $<TARGET_FILE:test_errors>  # disabled
      COMMAND $<TARGET_FILE:test_pool>
      DEPENDS test_token test_parse test_measure test_layout test_symbols test_pool
    )
  else()
    message(WARNING "ENABLE_HOST_TESTS=ON but PortCE or SDL2 not found - skipping tests")
  endif()
endif()

if(ENABLE_PORTCE_DEMO AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/portce)
  if(EMSCRIPTEN OR (NOT CMAKE_CROSSCOMPILING AND SDL2_FOUND))
    add_subdirectory(demo/portce)
  endif()
endif()

