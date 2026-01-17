set(DEMO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(NOT EMSCRIPTEN)
  add_executable(demo_text_portce
    ${DEMO_DIR}/demo_text.cpp
    $<TARGET_OBJECTS:tex_core>
  )

  target_include_directories(demo_text_portce PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/tex
    ${CMAKE_SOURCE_DIR}/include
  )

  target_compile_options(demo_text_portce PRIVATE
    -Wno-bit-int-extension
    -Wno-c99-extensions
    -Wno-c23-extensions
    -Wno-zero-length-array
    -Wno-old-style-cast
  )

  target_link_libraries(demo_text_portce PUBLIC PortCE -lm)

  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/appvar)
  if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    file(GLOB FONT_FILES "${CMAKE_SOURCE_DIR}/assets/*.8xv")
    foreach(font_file ${FONT_FILES})
      get_filename_component(font_name ${font_file} NAME)
      configure_file(${font_file} ${CMAKE_BINARY_DIR}/appvar/${font_name} COPYONLY)
    endforeach()
  endif()

  message(STATUS "PortCE demo enabled - run demo_text_portce from ${CMAKE_BINARY_DIR}")
endif()

if(EMSCRIPTEN)
  add_executable(demo_wasm
    ${DEMO_DIR}/demo_text.cpp
    $<TARGET_OBJECTS:tex_core>
  )

  target_include_directories(demo_wasm PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/tex
    ${CMAKE_SOURCE_DIR}/include
  )

  target_link_libraries(demo_wasm PUBLIC PortCE -lm)

  target_compile_options(demo_wasm PRIVATE
    -Wno-error
  )

  target_link_options(demo_wasm PUBLIC
    "SHELL:-sASYNCIFY"
    "SHELL:-sALLOW_MEMORY_GROWTH=1"
    "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/assets@/appvar"
  )

  set_target_properties(demo_wasm PROPERTIES
    SUFFIX ".html"
  )

  message(STATUS "Emscripten demo enabled - build with: emcmake cmake .. && make")
endif()
