cmake_minimum_required(VERSION 3.20)

project(matrix_app_core C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MATRIX_BUILD_CE "Build for TI-84 Plus CE via CEdevToolchain.cmake" OFF)
option(MATRIX_ENABLE_TESTS "Build native tests" ON)
option(MATRIX_STRICT_WARNINGS "Enable -Werror and extra warnings" ON)

set(MATRIX_CORE_DIR "${CMAKE_CURRENT_LIST_DIR}/core")

set(MATRIX_CORE_HEADERS
  ${MATRIX_CORE_DIR}/include/matrix_core/arena.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/config.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/error.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/explanation.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/latex.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/matrix.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/matrix_core.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/ops.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/rational.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/row_ops.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/row_reduction.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/slab.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/writer.hpp
)

set(MATRIX_CORE_SOURCES
  ${MATRIX_CORE_DIR}/src/det_detail.cpp
  ${MATRIX_CORE_DIR}/src/explanation.cpp
  ${MATRIX_CORE_DIR}/src/latex_view.cpp
  ${MATRIX_CORE_DIR}/src/matrix_view.cpp
  ${MATRIX_CORE_DIR}/src/ops_basic_view.cpp
  ${MATRIX_CORE_DIR}/src/ops_cramer.cpp
  ${MATRIX_CORE_DIR}/src/ops_det_ondemand.cpp
  ${MATRIX_CORE_DIR}/src/ops_det_replace_column_ondemand.cpp
  ${MATRIX_CORE_DIR}/src/ops_inverse_ondemand.cpp
  ${MATRIX_CORE_DIR}/src/ops_minor_cofactor_ondemand.cpp
  ${MATRIX_CORE_DIR}/src/ops_rref_ondemand.cpp
  ${MATRIX_CORE_DIR}/src/rational.cpp
  ${MATRIX_CORE_DIR}/src/row_ops.cpp
)

if(MATRIX_BUILD_CE)
  # Self-contained CE builds (no dependency on root CMake)
  include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/CEdevToolchain.cmake)

  cedev_add_program(
    TARGET matrix_core_smoke
    NAME   "MCORE"
    DESCRIPTION "Matrix core smoke test (build-only)"

    SOURCES
      ${MATRIX_CORE_SOURCES}
      ${CMAKE_CURRENT_LIST_DIR}/core/ce/smoke_main.cpp

    INCLUDE_DIRECTORIES
      ${MATRIX_CORE_DIR}/include

    CXX_MODE

    LIBLOAD
      keypadc
  )

  message(STATUS "Matrix core CE smoke build enabled")
  return()
endif()

# ---------------------------------------------------------------------------
# Native library + tests
# ---------------------------------------------------------------------------
function(matrix_core_apply target_name)
  target_include_directories(${target_name} PRIVATE ${MATRIX_CORE_DIR}/include)
  target_compile_features(${target_name} PRIVATE cxx_std_20)

  target_compile_options(${target_name} PRIVATE
    -fno-exceptions
    -fno-rtti
    -Wframe-larger-than=127
    -Wno-error=frame-larger-than
  )

  if(MATRIX_STRICT_WARNINGS)
    target_compile_options(${target_name} PRIVATE
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
  endif()
endfunction()

add_executable(matrix_profile_cramer
  ${MATRIX_CORE_SOURCES}
  ${MATRIX_CORE_HEADERS}
  ${MATRIX_CORE_DIR}/profile/profile_cramer.cpp
)
matrix_core_apply(matrix_profile_cramer)
target_compile_options(matrix_profile_cramer PRIVATE -fstack-usage)

if(MATRIX_ENABLE_TESTS)
  enable_testing()

  add_executable(test_rational
    ${MATRIX_CORE_SOURCES}
    ${MATRIX_CORE_HEADERS}
    ${MATRIX_CORE_DIR}/tests/test_rational.cpp
  )
  matrix_core_apply(test_rational)

  add_executable(test_matrix_ops
    ${MATRIX_CORE_SOURCES}
    ${MATRIX_CORE_HEADERS}
    ${MATRIX_CORE_DIR}/tests/test_matrix_ops.cpp
  )
  matrix_core_apply(test_matrix_ops)

  add_executable(test_rref
    ${MATRIX_CORE_SOURCES}
    ${MATRIX_CORE_HEADERS}
    ${MATRIX_CORE_DIR}/tests/test_rref.cpp
  )
  matrix_core_apply(test_rref)

  add_executable(test_det
    ${MATRIX_CORE_SOURCES}
    ${MATRIX_CORE_HEADERS}
    ${MATRIX_CORE_DIR}/tests/test_det.cpp
  )
  matrix_core_apply(test_det)

  add_executable(test_minor_cofactor
    ${MATRIX_CORE_SOURCES}
    ${MATRIX_CORE_HEADERS}
    ${MATRIX_CORE_DIR}/tests/test_minor_cofactor.cpp
  )
  matrix_core_apply(test_minor_cofactor)

  add_executable(test_cramer
    ${MATRIX_CORE_SOURCES}
    ${MATRIX_CORE_HEADERS}
    ${MATRIX_CORE_DIR}/tests/test_cramer.cpp
  )
  matrix_core_apply(test_cramer)

  add_executable(test_inverse
    ${MATRIX_CORE_SOURCES}
    ${MATRIX_CORE_HEADERS}
    ${MATRIX_CORE_DIR}/tests/test_inverse.cpp
  )
  matrix_core_apply(test_inverse)

  add_test(NAME rational COMMAND test_rational)
  add_test(NAME matrix_ops COMMAND test_matrix_ops)
  add_test(NAME rref COMMAND test_rref)
  add_test(NAME det COMMAND test_det)
  add_test(NAME minor_cofactor COMMAND test_minor_cofactor)
  add_test(NAME cramer COMMAND test_cramer)
  add_test(NAME inverse COMMAND test_inverse)
endif()
