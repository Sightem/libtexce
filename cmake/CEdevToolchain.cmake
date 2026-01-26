# -----------------------------------------------------------------------------
# CEdev Toolchain for TI‑84 Plus CE (CMake)
# -----------------------------------------------------------------------------
# this file makes the CEdev toolchain (ez80-clang, fasmg, convbin) available
# in CMake and provides the cedev_add_program() helper
#
# quick start:
#   cmake -S . -B build -DCEDEV_ROOT=/path/to/CEdev
#   cmake --build build --target demo
#
# minimal CMakeLists.txt example:
#   cmake_minimum_required(VERSION 3.20)
#   project(my_ce_program C CXX)
#
#   include(cmake/CEdevToolchain.cmake)
#
#   cedev_add_program(
#     TARGET demo
#     NAME "DEMO"
#     SOURCES src/main.cpp
#
#     INCLUDE_DIRECTORIES include src
#
#     # defaults to 20 if omitted
#     CXX_STANDARD 20
#
#     # stack & memory
#     LINKER_GLOBALS
#        "range .bss $D052C6 : $D13FD8"
#        "provide __stack = $D10000"
#
#     # libload libraries
#     LIBLOAD graphx keypadc
#   )
#
# toggles (pass to cmake -DNAME=ON/OFF):
#   CEDEV_HAS_PRINTF        Link toolchain printf instead of OS (default OFF)
#   CEDEV_PREFER_OS_LIBC    Prefer OS libc where possible (default ON)
#   CEDEV_ENABLE_LTO        Enable link-time optimization (default ON)
#   CEDEV_DEBUG_CONSOLE     Enable dbg_printf / keep assertions (default ON)
# -----------------------------------------------------------------------------

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

option(CEDEV_HAS_PRINTF       "Link toolchain printf implementations" OFF)
option(CEDEV_PREFER_OS_LIBC   "Prefer OS libc where possible" ON)
option(CEDEV_ENABLE_LTO         "Enable link-time optimization" ON)
option(CEDEV_DEBUG_CONSOLE      "Enable emulator debug console" ON)
option(CEDEV_IDE_HELPER         "Emit hidden OBJECT target for IDEs" ON)
option(CEDEV_DEFAULT_COMPRESS   "Package compressed .8xp by default" ON)
option(CEDEV_DEFAULT_ARCHIVED   "Mark program archived by default" ON)
option(CEDEV_STRICT_WARNINGS    "Enable aggressive warnings for ez80 sources" ON)
option(CEDEV_WARNINGS_AS_ERRORS "Treat warnings as errors for ez80 sources" ON)
option(CEDEV_FORCE_CXX_LANGUAGE "Force -x c++ when CEDEV_CXX_MODE is enabled" OFF)

if(NOT DEFINED CEDEV_ROOT)
  set(CEDEV_ROOT "/home/sightem/CEdev" CACHE PATH "Path to CEdev toolchain root")
endif()

set(CEDEV_BIN        "${CEDEV_ROOT}/bin")
set(CEDEV_INCLUDE    "${CEDEV_ROOT}/include")
set(CEDEV_LIB        "${CEDEV_ROOT}/lib")

set(CEDEV_C_COMPILER_DEFAULT   "${CEDEV_BIN}/ez80-clang")
set(CEDEV_CXX_COMPILER_DEFAULT "${CEDEV_BIN}/ez80-clang")
set(CEDEV_C_COMPILER   "${CEDEV_C_COMPILER_DEFAULT}" CACHE FILEPATH "C compiler for ez80")
set(CEDEV_CXX_COMPILER "${CEDEV_CXX_COMPILER_DEFAULT}" CACHE FILEPATH "C++ compiler for ez80")

set(CMAKE_C_COMPILER   "${CEDEV_C_COMPILER}")
set(CMAKE_CXX_COMPILER "${CEDEV_CXX_COMPILER}")
set(CEDEV_LINKER       "${CEDEV_BIN}/ez80-link")
set(CEDEV_CONVBIN      "${CEDEV_BIN}/convbin")

set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

if(NOT DEFINED ENV{PATH})
  set(ENV{PATH} "${CEDEV_BIN}")
else()
  set(ENV{PATH} "${CEDEV_BIN}:$ENV{PATH}")
endif()

set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES "${CEDEV_INCLUDE}")
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES "${CEDEV_INCLUDE}")
include_directories("${CEDEV_INCLUDE}")

set(_CEDEV_WARNINGS
  -Wall -Wextra
)
if(CEDEV_STRICT_WARNINGS)
  list(APPEND _CEDEV_WARNINGS
    -Wpedantic
    -Wconversion -Wsign-conversion
    -Wshadow -Wdouble-promotion
    -Wformat=2 -Wformat-security
    -Wcast-qual -Wpointer-arith
    -Wundef -Wnull-dereference
    -Wimplicit-fallthrough
  )
endif()
if(CEDEV_WARNINGS_AS_ERRORS)
  list(APPEND _CEDEV_WARNINGS -Werror)
endif()

add_compile_options(${_CEDEV_WARNINGS} -Oz)
add_compile_options(-ffunction-sections -fdata-sections)

if(CEDEV_ENABLE_LTO)
  add_link_options(--lto)
endif()
if(CEDEV_PREFER_OS_LIBC)
  add_link_options(--prefer-os-libc)
endif()

# cedev_add_program
function(cedev_add_program)
  set(options CXX_MODE)
  set(oneValueArgs TARGET NAME DESCRIPTION ICON COMPRESS MODE ARCHIVED CXX_STANDARD)
  set(multiValueArgs SOURCES LIBLOAD OPTIONAL_LIBS INCLUDE_DIRECTORIES COMPILE_OPTIONS LINKER_GLOBALS)
  cmake_parse_arguments(CEDEV "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT CEDEV_TARGET)
    message(FATAL_ERROR "cedev_add_program requires TARGET <name>")
  endif()

  if(CEDEV_CXX_MODE)
    set_source_files_properties(${CEDEV_SOURCES} PROPERTIES LANGUAGE CXX)
  endif()

  if(NOT CEDEV_NAME)
    string(TOUPPER "${CEDEV_TARGET}" CEDEV_NAME)
  endif()

  if(NOT DEFINED CEDEV_CXX_STANDARD)
    if(DEFINED CMAKE_CXX_STANDARD)
      set(CEDEV_CXX_STANDARD ${CMAKE_CXX_STANDARD})
    else()
      set(CEDEV_CXX_STANDARD 20)
    endif()
  endif()

  if(NOT CEDEV_LINKER_GLOBALS)
    set(CEDEV_LINKER_GLOBALS
      "range .bss $D052C6 : $D13FD8"
      "provide __stack = $D1987E"
      "locate .header at $D1A87F"
    )
  endif()

  set(BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CEDEV_TARGET}")
  set(OBJ_DIR   "${BUILD_DIR}/obj")
  set(BIN_DIR   "${BUILD_DIR}/bin")
  file(MAKE_DIRECTORY "${OBJ_DIR}")
  file(MAKE_DIRECTORY "${BIN_DIR}")

  # compile includes
  set(_custom_includes_flags)
  list(APPEND CEDEV_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}")
  
  foreach(_inc IN LISTS CEDEV_INCLUDE_DIRECTORIES)
    # resolve absolute path if relative provided
    get_filename_component(_abs_inc "${_inc}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    list(APPEND _custom_includes_flags "-I${_abs_inc}")
  endforeach()

  # compile options
  set(_user_compile_options)
  if(CEDEV_COMPILE_OPTIONS)
    set(_user_compile_options ${CEDEV_COMPILE_OPTIONS})
  endif()

  set(COMPILED_SRCS)
  foreach(SRC IN LISTS CEDEV_SOURCES)
    get_filename_component(SRC_NAME "${SRC}" NAME)
    get_filename_component(SRC_EXT  "${SRC}" EXT)
    set(OUT_SRC "${OBJ_DIR}/${SRC_NAME}.src")
    get_filename_component(ABS_SRC "${SRC}" ABSOLUTE)

    set(_compiler "${CMAKE_C_COMPILER}")
    set(_std_flags)
    set(_lang_specific_flags)
    set(_is_cxx 0)

    if(SRC_EXT STREQUAL ".cpp" OR SRC_EXT STREQUAL ".cxx" OR SRC_EXT STREQUAL ".cc")
      set(_is_cxx 1)
    endif()
    if(CEDEV_CXX_MODE)
      set(_is_cxx 1)
    endif()

    if(_is_cxx)
      set(_compiler "${CMAKE_CXX_COMPILER}")
      set(_std_flags "-std=c++${CEDEV_CXX_STANDARD}")
      if(CEDEV_FORCE_CXX_LANGUAGE)
        list(APPEND _std_flags -xc++)
      endif()
      set(_lang_specific_flags 
          -fno-exceptions 
          -fno-use-cxa-atexit 
          -DEASTL_USER_CONFIG_HEADER=<__EASTL_user_config.h> 
          -isystem "${CEDEV_INCLUDE}/c++"
      )
    endif()

    set(_debug_flags)
    if(CEDEV_DEBUG_CONSOLE)
      set(_debug_flags -UNDEBUG)
    else()
      set(_debug_flags -DNDEBUG)
    endif()

    set(_base_flags
      -target ez80-none-elf
      -S -nostdinc
      -isystem "${CEDEV_INCLUDE}"
      -fno-threadsafe-statics
      -fno-addrsig
      # Note: -Xclang -fforce-mangle-main-argc-argv removed for v17+ compatibility
      -D__TICE__ 
      ${_CEDEV_WARNINGS} -Oz
    )

    set(_depfile "${OUT_SRC}.d")
    add_custom_command(
      OUTPUT "${OUT_SRC}"
      DEPFILE "${_depfile}"
      BYPRODUCTS "${_depfile}"
      COMMAND "${CMAKE_COMMAND}" -E echo "[compiling] ${SRC}"
      COMMAND "${_compiler}"
              ${_base_flags}
              ${_std_flags}
              ${_lang_specific_flags}
              ${_debug_flags}
              ${_custom_includes_flags}
              ${_user_compile_options}
              -MMD -MF "${_depfile}" -MT "${OUT_SRC}"
              "${ABS_SRC}" -o "${OUT_SRC}"
      DEPENDS "${ABS_SRC}"
      VERBATIM
    )
    list(APPEND COMPILED_SRCS "${OUT_SRC}")
  endforeach()

  set(LIBLOAD_FILES)
  foreach(LIB_NAME IN LISTS CEDEV_LIBLOAD)
    set(LIB_FILE "${CEDEV_LIB}/libload/${LIB_NAME}.lib")
    if(NOT EXISTS "${LIB_FILE}")
      message(FATAL_ERROR "Requested LIBLOAD library not found: ${LIB_FILE}")
    endif()
    list(APPEND LIBLOAD_FILES "${LIB_FILE}")
  endforeach()

  set(LINKER_SCRIPT "${CEDEV_ROOT}/meta/linker_script")
  set(LD_ALM        "${CEDEV_ROOT}/meta/ld.alm")
  set(CRT0          "${CEDEV_ROOT}/lib/crt/crt0.src")

  set(LIBLIST)
  foreach(LIB_FILE IN LISTS LIBLOAD_FILES)
    if(LIBLIST)
      set(LIBLIST "${LIBLIST}, \"${LIB_FILE}\"")
    else()
      set(LIBLIST "\"${LIB_FILE}\"")
    endif()
  endforeach()

  set(TARGET_BIN "${BIN_DIR}/${CEDEV_NAME}.bin")
  set(TARGET_8XP "${BIN_DIR}/${CEDEV_NAME}.8xp")

  if(CEDEV_HAS_PRINTF)
    set(_HAS_PRINTF 1)
  else()
    set(_HAS_PRINTF 0)
  endif()
  if(CEDEV_PREFER_OS_LIBC)
    set(_PREF_OS_LIBC 1)
  else()
    set(_PREF_OS_LIBC 0)
  endif()
  if(CEDEV_DEBUG_CONSOLE)
    set(_DBG 1)
  else()
    set(_DBG 0)
  endif()

  # Build FASMG Arguments
  set(FASMG_ARGS
    "${LD_ALM}"
    -i "DEBUG := ${_DBG}"
    -i "HAS_PRINTF := ${_HAS_PRINTF}"
    -i "HAS_LIBC := 1"
    -i "HAS_LIBCXX := 1"
    -i "HAS_EASTL := 1"
    -i "PREFER_OS_CRT := 0"
    -i "PREFER_OS_LIBC := ${_PREF_OS_LIBC}"
    -i "ALLOCATOR_STANDARD := 1"
    -i "__TICE__ := 1"
    -i "include \"${LINKER_SCRIPT}\""
  )

  # inject User Mmemory map and globals
  foreach(_global IN LISTS CEDEV_LINKER_GLOBALS)
    list(APPEND FASMG_ARGS -i "${_global}")
  endforeach()

  if(LIBLIST)
    list(APPEND FASMG_ARGS -i "library ${LIBLIST}")
  endif()

  set(SOURCE_LIST "\"${CRT0}\"")
  foreach(S IN LISTS COMPILED_SRCS)
    set(SOURCE_LIST "${SOURCE_LIST}, \"${S}\"")
  endforeach()
  list(APPEND FASMG_ARGS -i "source ${SOURCE_LIST}")

  add_custom_command(
    OUTPUT "${TARGET_BIN}"
    COMMAND "${CMAKE_COMMAND}" -E echo "[linking] ${TARGET_BIN}"
    COMMAND "${CEDEV_BIN}/fasmg" ${FASMG_ARGS} "${TARGET_BIN}"
    DEPENDS ${COMPILED_SRCS}
    VERBATIM
  )

  # compile_commands.json stuff
  if(CEDEV_IDE_HELPER)
    set(_ide_target "${CEDEV_TARGET}__compiledb")
    add_library(${_ide_target} OBJECT EXCLUDE_FROM_ALL ${CEDEV_SOURCES})
    
    target_include_directories(${_ide_target} PRIVATE 
      "${CEDEV_INCLUDE}" 
      "${CEDEV_INCLUDE}/c++" 
      ${CEDEV_INCLUDE_DIRECTORIES} # user provided includes
    )

    set(_ide_cxx_flags)
    if(CEDEV_CXX_STANDARD)
       set(_ide_cxx_flags "-std=c++${CEDEV_CXX_STANDARD}")
    endif()

    target_compile_options(${_ide_target} PRIVATE
      -nostdinc -fno-threadsafe-statics -D__TICE__ ${_debug_flags} ${_CEDEV_WARNINGS} -Oz
      $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<BOOL:${CEDEV_CXX_MODE}>>:${_ide_cxx_flags} $<$<BOOL:${CEDEV_FORCE_CXX_LANGUAGE}>:-xc++> -fno-use-cxa-atexit -DEASTL_USER_CONFIG_HEADER=<__EASTL_user_config.h>>
      ${_user_compile_options}
    )
  endif()

  # convbin
  set(CONVBIN_ARGS -k 8xp -n "${CEDEV_NAME}")
  set(USE_ARCHIVED ${CEDEV_ARCHIVED})
  set(USE_COMPRESS ${CEDEV_COMPRESS})

  if(NOT DEFINED USE_ARCHIVED)
    set(USE_ARCHIVED ${CEDEV_DEFAULT_ARCHIVED})
  endif()
  if(NOT DEFINED USE_COMPRESS)
    set(USE_COMPRESS ${CEDEV_DEFAULT_COMPRESS})
  endif()

  if(USE_ARCHIVED)
    list(APPEND CONVBIN_ARGS -r)
  endif()
  if(USE_COMPRESS)
    if(CEDEV_MODE)
      list(APPEND CONVBIN_ARGS -e ${CEDEV_MODE} -k 8xp-compressed)
    else()
      list(APPEND CONVBIN_ARGS -e zx7 -k 8xp-compressed)
    endif()
  endif()

  add_custom_command(
    OUTPUT "${TARGET_8XP}"
    COMMAND "${CMAKE_COMMAND}" -E echo "[convbin] ${TARGET_8XP}"
    COMMAND "${CEDEV_CONVBIN}" ${CONVBIN_ARGS} -i "${TARGET_BIN}" -o "${TARGET_8XP}"
    DEPENDS "${TARGET_BIN}"
    VERBATIM
  )

  add_custom_target(${CEDEV_TARGET} ALL DEPENDS "${TARGET_8XP}")

  add_custom_command(TARGET ${CEDEV_TARGET} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E echo "Built: ${TARGET_8XP}"
  )
endfunction()
