##
##  Copyright 2016-2019 libfptu authors: please see AUTHORS file.
##
##  This file is part of libfptu, aka "Fast Positive Tuples".
##
##  libfptu is free software: you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation, either version 3 of the License, or
##  (at your option) any later version.
##
##  libfptu is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with libfptu.  If not, see <http://www.gnu.org/licenses/>.
##

##
##  libfptu = { Fast Positive Tuples, aka Позитивные Кортежи }
##
##  The kind of lightweight linearized tuples, which are extremely handy
##  to machining, including cases with shared memory.
##  Please see README.md at https://github.com/leo-yuriev/libfptu
##
##  The Future will Positive. Всё будет хорошо.
##
##  "Позитивные Кортежи" дают легковесное линейное представление небольших
##  JSON-подобных структур в экстремально удобной для машины форме,
##  в том числе при размещении в разделяемой памяти.
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
  fetch_version(FPTU "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION")
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

set(LIBFPTU_SOURCES
  ${FAST_POSITIVE_CONFIG_H}
  ../fast_positive/details/erthink/erthink_arch.h
  ../fast_positive/details/erthink/erthink_bswap.h
  ../fast_positive/details/erthink/erthink_byteorder.h
  ../fast_positive/details/erthink/erthink_d2a.h
  ../fast_positive/details/erthink/erthink_defs.h
  ../fast_positive/details/erthink/erthink_dynamic_constexpr.h
  ../fast_positive/details/erthink/erthink_u2a.h
  ../fast_positive/details/erthink/erthink_short_alloc.h
  ../fast_positive/details/erthink/erthink_optimize4size.h
  ../fast_positive/details/erthink/erthink_optimize4speed.h
  ../fast_positive/details/erthink/erthink_ifunc.h
  ../fast_positive/details/erthink/erthink_misc.h
  ../fast_positive/details/erthink/erthink.h

  ../fast_positive/details/1Hippeus/actor.h
  ../fast_positive/details/1Hippeus/buffer.h
  ../fast_positive/details/1Hippeus/hipagut.h
  ../fast_positive/details/1Hippeus/hipagut_allocator.h
  ../fast_positive/details/1Hippeus/utils.h

  ../fast_positive/details/api.h
  ../fast_positive/details/audit.h
  ../fast_positive/details/fixed_binary.h
  ../fast_positive/details/bug.h
  ../fast_positive/details/cpu_features.h
  ../fast_positive/details/datetime.h
  ../fast_positive/details/decimal.h
  ../fast_positive/details/essentials.h
  ../fast_positive/details/exceptions.h
  ../fast_positive/details/field.h
  ../fast_positive/details/float.h
  ../fast_positive/details/getter.h
  ../fast_positive/details/integer128.h
  ../fast_positive/details/ip_address.h
  ../fast_positive/details/legacy_common.h
  ../fast_positive/details/legacy_compat.h
  ../fast_positive/details/mac_address.h
  ../fast_positive/details/meta.h
  ../fast_positive/details/nan.h
  ../fast_positive/details/property.h
  ../fast_positive/details/ro.h
  ../fast_positive/details/rw.h
  ../fast_positive/details/scan.h
  ../fast_positive/details/schema.h
  ../fast_positive/details/string_view.h
  ../fast_positive/details/tagged_pointer.h
  ../fast_positive/details/token.h
  ../fast_positive/details/type2genus.h
  ../fast_positive/details/types.h
  ../fast_positive/details/uio.h
  ../fast_positive/details/utils.h
  ../fast_positive/details/uuid.h
  ../fast_positive/details/crutches.h
  ../fast_positive/details/windows_mustdie.h
  ../fast_positive/details/posix_modern.h
  ../fast_positive/details/warnings_push_pt.h
  ../fast_positive/details/warnings_push_system.h
  ../fast_positive/details/warnings_pop.h
  ../fast_positive/tuples.h
  ../fast_positive/tuples_internal.h
  ../fast_positive/tuples_legacy.h

  1Hippeus/allot_stdcxx.cxx
  1Hippeus/buffer.cxx
  1Hippeus/hipagut.cxx
  1Hippeus/utils.cxx

  ../fast_positive/details/TODO/casting.h
  ../fast_positive/details/TODO/endianness.h
  ../fast_positive/details/TODO/unaligned.h
  TODO/json_parse.cxx
  TODO/json_emit.cxx
  TODO/magic.cxx

  autogrowth.cxx
  audit.cxx
  tablets.cxx
  debug.cxx
  except.cxx
  gear.cxx
  gperf_ECMAScript_keywords.h
  compat_common.cxx
  compat_field.cxx
  compat_ro.cxx
  compat_rw.cxx
  misc.cxx
  ro.cxx
  rw.cxx
  scan_ifunc.cxx
  scan_portable.cxx
  time.cxx

  obsolete/internal.h
  obsolete/bitset4tags.h
  obsolete/check.cxx
  obsolete/common.cxx
  obsolete/compare.cxx
  obsolete/create.cxx
  obsolete/data.cxx
  obsolete/get.cxx
  obsolete/iterator.cxx
  obsolete/remove.cxx
  obsolete/shrink.cxx
  obsolete/sort.cxx
  obsolete/upsert.cxx

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
  add_library(fptu_objects OBJECT ${LIBFPTU_SOURCES})
  add_library(fptu INTERFACE)
  target_compile_features(fptu_objects INTERFACE "cxx_std_${FPTU_CXX_STANDARD}")
  add_dependencies(fptu fptu_objects)
  target_link_libraries(fptu INTERFACE $<TARGET_OBJECTS:fptu_objects>)
  set(fptu_target fptu_objects)
else()
  add_library(fptu ${LIBFPTU_TYPE} ${LIBFPTU_SOURCES})
  set(fptu_target fptu)
endif()

set_target_properties(${fptu_target} PROPERTIES
  PROJECT_LABEL "Fast Positive Tuples"
  FRAMEWORK TRUE
  VERSION "${FPTU_VERSION}"
  PUBLIC_HEADER "../fast_positive/defs.h;../fast_positive/tuples.h;${FAST_POSITIVE_CONFIG_H}"
  PRIVATE_HEADER ../fast_positive/tuples_internal.h
  INTERPROCEDURAL_OPTIMIZATION $<BOOL:${INTERPROCEDURAL_OPTIMIZATION}>
  CXX_STANDARD ${FPTU_CXX_STANDARD}
  CXX_STANDARD_REQUIRED ON
  C_STANDARD 11
  C_STANDARD_REQUIRED OFF
  )

if(CC_HAS_FASTMATH)
  target_compile_options(${fptu_target} PRIVATE "-ffast-math")
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

install(TARGETS fptu
  LIBRARY DESTINATION lib COMPONENT runtime
  RUNTIME DESTINATION lib COMPONENT runtime
  ARCHIVE DESTINATION lib/static COMPONENT devel
  PUBLIC_HEADER DESTINATION include/fast_positive INCLUDES DESTINATION include COMPONENT devel
  PRIVATE_HEADER DESTINATION include/fast_positive INCLUDES DESTINATION include COMPONENT devel
  )

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
  add_definitions(-DFPTU_BUILD_TAGRET="${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-$<CONFIG>")
  set(FPTU_BUILD_TAGRET FPTU_BUILD_TAGRET)
else()
  string(STRIP "\"${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_BUILD_TYPE}\"" FPTU_BUILD_TAGRET)
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

set(options VERSION BUILD C_COMPILER CXX_COMPILER DEFINITIONS
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

if(NOT BUILD_SHARED_LIBS)
  message(
    "   **********************************************************************\n"
    "       libfptu is free software: you can redistribute it and/or modify it\n"
    "       under the terms of the GNU Lesser General Public License as\n"
    "       published by the Free Software Foundation, either version 3 of the\n"
    "       License, or (at your option) any later version.\n\n"
    "       So, libfptu could be STATICALLY LINKED ONLY WITH GPLed software.\n"
    "   **********************************************************************")
endif()
