##
##  Fast Positive Tuples (libfptu), aka Позитивные Кортежи
##  Copyright 2016-2020 Leonid Yuriev <leo@yuriev.ru>
##
##  Licensed under the Apache License, Version 2.0 (the "License");
##  you may not use this file except in compliance with the License.
##  You may obtain a copy of the License at
##
##      http://www.apache.org/licenses/LICENSE-2.0
##
##  Unless required by applicable law or agreed to in writing, software
##  distributed under the License is distributed on an "AS IS" BASIS,
##  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
##  See the License for the specific language governing permissions and
##  limitations under the License.
##

##
##  libfptu = { Fast Positive Tuples, aka Позитивные Кортежи }
##  Машинно-эффективный формат линейного представления небольших структур данных
##  для (де)сериализации, обмена сообщениями и размещения в разделяемой памяти.
##
##  Всё будет хорошо. The Future will (be) Positive.
##  https://github.com/PositiveTechnologies/libfptu
##
##  Machine-handy format for linear representation of small data structures
##  for (de)serialization, messaging and placement in shared memory.
##

find_program(RE2C re2c)
if(RE2C AND NOT RE2C_VERSION)
  execute_process(COMMAND ${RE2C} --version
    OUTPUT_VARIABLE re2c_version_output
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE rc)
  if(rc EQUAL 0)
    string(REGEX MATCH "^re2c ([0-9]+\\.[0-9]+(\\.[0-9]+)*)" re2c_version_valid ${re2c_version_output})
    if(re2c_version_valid)
      set(RE2C_VERSION ${CMAKE_MATCH_1})
      message(STATUS "Found ${RE2C} version ${RE2C_VERSION}")
    endif()
  endif()
endif()

#
# Get version
if(HAVE_FPTU_VERSIONINFO)
  fetch_version(FPTU "${CMAKE_CURRENT_SOURCE_DIR}/.." TRUE)
  message(STATUS "libfptu version is ${FPTU_VERSION}")
else()
  set(FPTU_VERSION "unversioned")
endif()

if(NOT LIBFPTU_TYPE)
  if(BUILD_SHARED_LIBS)
    set(LIBFPTU_TYPE SHARED)
  else()
    set(LIBFPTU_TYPE STATIC)
  endif()
endif()

set(LIBFPTU_HEADERS
  tuples.h
  tuples/api.h
  tuples/essentials.h
  tuples/legacy.h
  tuples/schema.h
  tuples/string_view.h
  tuples/token.h
  tuples/internal.h

  tuples/types.h
  tuples/types/datetime.h
  tuples/types/decimal.h
  tuples/types/fixed_binary.h
  tuples/types/float.h
  tuples/types/integer128.h
  tuples/types/ip_address.h
  tuples/types/mac_address.h
  tuples/types/property.h
  tuples/types/uuid.h

  erthink/erthink_arch.h
  erthink/erthink_bswap.h
  erthink/erthink_byteorder.h
  erthink/erthink_d2a.h
  erthink/erthink_defs.h
  erthink/erthink_dynamic_constexpr.h
  erthink/erthink_u2a.h
  erthink/erthink_short_alloc.h
  erthink/erthink_optimize4size.h
  erthink/erthink_optimize4speed.h
  erthink/erthink_ifunc.h
  erthink/erthink_misc.h
  erthink/erthink_endian.h

  tuples/1Hippeus/actor.h
  tuples/1Hippeus/buffer.h
  tuples/1Hippeus/hipagut.h
  tuples/1Hippeus/hipagut_allocator.h
  tuples/1Hippeus/utils.h

  tuples/details/audit.h
  tuples/details/exceptions.h
  tuples/details/field.h
  tuples/details/getter.h
  tuples/details/legacy_common.h
  tuples/details/legacy_compat.h
  tuples/details/meta.h
  tuples/details/ro.h
  tuples/details/rw.h
  tuples/details/scan.h
  tuples/details/tagged_pointer.h
  tuples/details/type2genus.h
  tuples/details/uio.h
  tuples/details/utils.h
  tuples/details/windows_mustdie.h
  tuples/details/posix_modern.h
  tuples/details/bug.h
  tuples/details/cpu_features.h
  tuples/details/crutches.h
  tuples/details/nan.h

  tuples/details/warnings_push_pt.h
  tuples/details/warnings_push_system.h
  tuples/details/warnings_pop.h
  )

foreach(file ${LIBFPTU_HEADERS})
  list(APPEND LIBFPTU_HEADERS_PRETENDED "../fast_positive/${file}")
endforeach()

set(LIBFPTU_SOURCES
  1Hippeus/allot_stdcxx.cxx
  1Hippeus/buffer.cxx
  1Hippeus/hipagut.cxx
  1Hippeus/utils.cxx

  ../fast_positive/tuples/TODO/casting.h
  ../fast_positive/tuples/TODO/unaligned.h
  TODO/json_parse.cxx
  TODO/json_emit.cxx
  TODO/magic.cxx

  audit.cxx
  autogrowth.cxx
  debug.cxx
  except.cxx
  gear.cxx
  gperf_ECMAScript_keywords.h
  compat_common.cxx
  compat_field.cxx
  compat_ro.cxx
  compat_rw.cxx
  managed.cxx
  misc.cxx
  ro.cxx
  rw.cxx
  scan_ifunc.cxx
  scan_portable.cxx
  schema.cxx
  tablets.cxx
  time.cxx

  ${CMAKE_CURRENT_BINARY_DIR}/version.cxx)

if(X86_32 OR X86_64)
  list(APPEND LIBFPTU_SOURCES cpu_features_ia32.cxx scan_SSE2.cxx scan_AVX.cxx scan_AVX2.cxx)
  if(MSVC)
    if(X86_32)
      set_source_files_properties(scan_SSE2.cxx COMPILE_FLAGS /arch:SSE2)
    endif()
    set_source_files_properties(scan_AVX.cxx COMPILE_FLAGS /arch:AVX)
    set_source_files_properties(scan_AVX2.cxx COMPILE_FLAGS /arch:AVX2)
  else()
    if(X86_32)
      set_source_files_properties(scan_SSE2.cxx COMPILE_FLAGS -msse2)
    endif()
    set_source_files_properties(scan_AVX.cxx COMPILE_FLAGS -mavx)
    set_source_files_properties(scan_AVX2.cxx COMPILE_FLAGS -mavx2)
  endif()
elseif(E2K)
  list(APPEND LIBFPTU_SOURCES scan_AVX.cxx)
endif()

if(CMAKE_COMPILER_IS_GNUCC)
  set_source_files_properties(ro.cxx COMPILE_FLAGS -save-temps)
  set_source_files_properties(rw.cxx COMPILE_FLAGS -save-temps)
endif()

if(LIBFPTU_TYPE STREQUAL "PILE")
  add_library(fptu_objects OBJECT ${LIBFPTU_SOURCES} ${LIBFPTU_HEADERS_PRETENDED} ${LIBFPTU_CONFIG_H})
  add_library(fptu INTERFACE)
  target_compile_features(fptu_objects INTERFACE "cxx_std_${FPTU_CXX_STANDARD}")
  add_dependencies(fptu fptu_objects)
  target_link_libraries(fptu INTERFACE $<TARGET_OBJECTS:fptu_objects>)
  set(fptu_target fptu_objects)
else()
  add_library(fptu ${LIBFPTU_TYPE} ${LIBFPTU_SOURCES} ${LIBFPTU_HEADERS_PRETENDED} ${LIBFPTU_CONFIG_H})
  set(fptu_target fptu)
endif()

set_target_properties(${fptu_target} PROPERTIES
  PROJECT_LABEL "Fast Positive Tuples"
  VERSION "${FPTU_VERSION}"
  PUBLIC_HEADER "${LIBFPTU_CONFIG_H}"
#  PRIVATE_HEADER "${LIBFPTU_PRIVATE_INCLUDES}"
  INTERPROCEDURAL_OPTIMIZATION $<BOOL:${INTERPROCEDURAL_OPTIMIZATION}>
  CXX_STANDARD ${FPTU_CXX_STANDARD}
  CXX_STANDARD_REQUIRED ON
  C_STANDARD 11
  C_STANDARD_REQUIRED OFF
  )

if(CC_HAS_FASTMATH)
  target_compile_options(${fptu_target} PRIVATE "-ffast-math")
endif()

if(BUILD_FOR_NATIVE_CPU AND CC_HAS_ARCH_NATIVE)
  target_compile_options(${fptu_target} PUBLIC "-march=native")
endif()

if(CC_HAS_VISIBILITY)
  target_compile_options(${fptu_target} PRIVATE "-fvisibility=hidden")
  if(LTO_ENABLED OR INTERPROCEDURAL_OPTIMIZATION)
    set_target_properties(${fptu_target} PROPERTIES LINK_FLAGS "-fvisibility=hidden")
  endif()
endif()

target_compile_features(fptu INTERFACE "cxx_std_${FPTU_CXX_STANDARD}")
target_link_libraries(fptu INTERFACE ${LIB_MATH} ${CMAKE_THREAD_LIBS_INIT})
if(UNIX AND NOT APPLE)
  target_link_libraries(fptu INTERFACE rt)
endif()

if(LIBFPTU_TYPE STREQUAL "SHARED")
  target_compile_definitions(${fptu_target} PRIVATE "fptu_EXPORTS" INTERFACE "fptu_IMPORTS")
endif()

if(CMAKE_VERSION VERSION_LESS 3.12)
  install(TARGETS fptu EXPORT fptu
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION bin COMPONENT runtime
    OBJECTS DESTINATION lib COMPONENT devel
    ARCHIVE DESTINATION lib COMPONENT devel
    PUBLIC_HEADER DESTINATION include/fast_positive/tuples INCLUDES DESTINATION include COMPONENT devel
  #  PRIVATE_HEADER DESTINATION include/fast_positive/ INCLUDES DESTINATION include COMPONENT devel
    INCLUDES DESTINATION include/fast_positive/tuples COMPONENT devel)
else()
  install(TARGETS fptu EXPORT fptu
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION bin COMPONENT runtime
    NAMELINK_COMPONENT devel
    OBJECTS DESTINATION lib COMPONENT devel
    ARCHIVE DESTINATION lib COMPONENT devel
    PUBLIC_HEADER DESTINATION include/fast_positive/tuples INCLUDES DESTINATION include COMPONENT devel
  #  PRIVATE_HEADER DESTINATION include/fast_positive/ INCLUDES DESTINATION include COMPONENT devel
    INCLUDES DESTINATION include/fast_positive/tuples COMPONENT devel)
endif()

foreach(file ${LIBFPTU_HEADERS} erthink/LICENSE erthink/AUTHORS)
  get_filename_component(dir ${file} DIRECTORY)
  install(FILES "../fast_positive/${file}" DESTINATION "include/fast_positive/${dir}")
endforeach()

foreach(file LICENSE README.md)
  install(FILES "../${file}" DESTINATION "include/fast_positive/tuples")
endforeach()

################################################################################
#
# library build info (used in library version output)
#

# get definitions as a string of "-Dxyz=124 ..."
get_target_property(FPTU_DEFINITIONS ${fptu_target} COMPILE_DEFINITIONS)
if(NOT FPTU_DEFINITIONS)
  set(FPTU_DEFINITIONS "")
endif()
list(REMOVE_DUPLICATES FPTU_DEFINITIONS)
string(REGEX REPLACE "([^;]+)" " -D\\1" FPTU_DEFINITIONS "${FPTU_DEFINITIONS}")
string(STRIP FPTU_DEFINITIONS "${FPTU_DEFINITIONS}")

# get target compile options as a list
get_target_property(fptu_compile_options ${fptu_target} COMPILE_OPTIONS)
if(NOT fptu_compile_options)
  set(fptu_compile_options "")
endif()

# append cmake's common cxx flags and defines
string(REPLACE " " ";" cmake_cxx_options "${CMAKE_CXX_FLAGS}" "${CMAKE_CXX_DEFINES}")
list(INSERT fptu_compile_options 0 "${cmake_cxx_options}")
unset(cmake_cxx_options)

# append cmake's build-type flags and defines
if(NOT CMAKE_CONFIGURATION_TYPES)
  string(REPLACE " " ";" cmake_cxx_options "${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPERCASE}}" "${CMAKE_CXX_DEFINES_${CMAKE_BUILD_TYPE_UPPERCASE}}")
  list(APPEND fptu_compile_options "${cmake_cxx_options}")
  unset(cmake_cxx_options)
endif()

# append linker dll's options
if(LIBFPTU_TYPE STREQUAL "SHARED")
  string(REPLACE " " ";" cmake_shared_linker_options "${CMAKE_SHARED_LINKER_FLAGS}")
  list(APPEND fptu_compile_options ${cmake_shared_linker_options})
  unset(cmake_shared_linker_options)
endif()

# drop duplicates in the option list
list(REMOVE_DUPLICATES fptu_compile_options)

# make string of space separated flags
string(REPLACE ";" " " FPTU_COMPILE_FLAGS "${fptu_compile_options}")
unset(fptu_compile_options)
string(STRIP "${FPTU_COMPILE_FLAGS}${FPTU_DEFINITIONS}" FPTU_COMPILE_FLAGS)
if(CMAKE_CONFIGURATION_TYPES)
  # add dynamic part via per-configuration define
  message(STATUS "FPTU Compile Flags: ${FPTU_COMPILE_FLAGS} <AND CONFIGURATION DEPENDENT>")
  set(FPTU_COMPILE_FLAGS "FPTU_COMPILE_FLAGS \"${FPTU_COMPILE_FLAGS}\"")
  add_definitions(
    -DFPTU_COMPILE_FLAGS="$<$<CONFIG:Debug>:${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_CXX_DEFINES_DEBUG}>$<$<CONFIG:Release>:${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_CXX_DEFINES_RELEASE}>$<$<CONFIG:RelWithDebInfo>:${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${CMAKE_CXX_DEFINES_RELWITHDEBINFO}>$<$<CONFIG:MinSizeRel>:${CMAKE_CXX_FLAGS_MINSIZEREL} ${CMAKE_CXX_DEFINES_MINSIZEREL}>"
    )
else()
  message(STATUS "FPTU Compile Flags: ${FPTU_COMPILE_FLAGS}")
  set(FPTU_COMPILE_FLAGS "\"${FPTU_COMPILE_FLAGS}\"")
endif()

# make a build-target triplet
if(CMAKE_CONFIGURATION_TYPES)
  # via per-configuration define
  add_definitions(-DFPTU_BUILD_TARGET="${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-$<CONFIG>")
  set(FPTU_BUILD_TARGET "FPTU_BUILD_TARGET")
else()
  string(STRIP "\"${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_BUILD_TYPE}\"" FPTU_BUILD_TARGET)
endif()

# generate version file
string(TIMESTAMP FPTU_BUILD_TIMESTAMP UTC)
string(REPLACE "  " " " FPTU_CMAKE_OPTIONS "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}"
  " -DENABLE_ASAN=${ENABLE_ASAN}"
  " -DENABLE_VALGRIND=${ENABLE_VALGRIND}"
  " -DENABLE_GPROF=${ENABLE_GPROF}"
  " -DENABLE_GCOV=${ENABLE_GCOV}"
  " -DENABLE_BACKTRACE=${ENABLE_BACKTRACE}"
  )

string(STRIP "${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}" FPTU_BUILD_COMPILER)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.cxx.in ${CMAKE_CURRENT_BINARY_DIR}/version.cxx)

set(options VERSION C_COMPILER CXX_COMPILER DEFINITIONS
  ENABLE_GCOV ENABLE_GPROF ENABLE_VALGRIND ENABLE_BACKTRACE BUILD_TARGET
  HAVE_BFD ENABLE_ASAN)
foreach(option IN LISTS options)
  if(DEFINED FPTU_${option})
    set(value ${FPTU_${option}})
  elseif(DEFINED ${option})
    set(value "${${option}}")
  else()
    set(value "${CMAKE_${option}}")
  endif()
  message(STATUS "FPTU_${option}: ${value}")
endforeach(option)
