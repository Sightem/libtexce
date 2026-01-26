set(MATRIX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(NOT EMSCRIPTEN)
  add_executable(matrix_portce
    ${MATRIX_DIR}/src/main_portce.cpp
    ${MATRIX_DIR}/src/app.cpp
    ${MATRIX_DIR}/src/app_draw.cpp
    ${MATRIX_DIR}/src/input.cpp
    ${MATRIX_DIR}/src/rational.cpp
    ${MATRIX_DIR}/src/matrix_ops.cpp
    ${MATRIX_DIR}/src/steps.cpp
    ${MATRIX_DIR}/src/ui_menu.cpp
    ${MATRIX_DIR}/src/screens_main_menu.cpp
    ${MATRIX_DIR}/src/screens_matrices_menu.cpp
    ${MATRIX_DIR}/src/screens_operations_menu.cpp
    ${MATRIX_DIR}/src/screens_slot_picker.cpp
    ${MATRIX_DIR}/src/screens_result_view.cpp
    ${MATRIX_DIR}/src/screens_steps_menu.cpp
    ${MATRIX_DIR}/src/screens_steps_view.cpp
    ${MATRIX_DIR}/src/screens_matrix_resize.cpp
    ${MATRIX_DIR}/src/screens_matrix_editor.cpp
    ${MATRIX_DIR}/src/screens_element_picker.cpp
    ${MATRIX_DIR}/src/screens_element_output.cpp
    ${MATRIX_DIR}/src/screens_stub.cpp
    ${MATRIX_DIR}/src/screens_dispatch.cpp
    $<TARGET_OBJECTS:tex_core>
  )

  target_include_directories(matrix_portce PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/tex
    ${CMAKE_SOURCE_DIR}/include
    ${MATRIX_DIR}/src
  )

  target_compile_options(matrix_portce PRIVATE
    -Wno-bit-int-extension
    -Wno-c99-extensions
    -Wno-c23-extensions
    -Wno-zero-length-array
    -Wno-old-style-cast
    -Wno-error
    -Wno-switch-enum
    -Wno-implicit-int-conversion
  )

  target_link_libraries(matrix_portce PUBLIC PortCE -lm)

  # The repo's native preset enables clang-tidy with warnings-as-errors; disable
  # per-target so PortCE typedefs (_BitInt(24)) don't block building.
  set_target_properties(matrix_portce PROPERTIES C_CLANG_TIDY "" CXX_CLANG_TIDY "")

  # Copy font appvars to build directory for PortCE runtime.
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/appvar)
  if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    file(GLOB FONT_FILES "${CMAKE_SOURCE_DIR}/assets/*.8xv")
    foreach(font_file ${FONT_FILES})
      get_filename_component(font_name ${font_file} NAME)
      configure_file(${font_file} ${CMAKE_BINARY_DIR}/appvar/${font_name} COPYONLY)
    endforeach()
  endif()

  message(STATUS "PortCE matrix enabled - run matrix_portce from ${CMAKE_BINARY_DIR}")
endif()
