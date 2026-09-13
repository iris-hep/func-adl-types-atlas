cmake_minimum_required(VERSION 3.20.0)
project(func_adl_types_atlas VERSION 0.1.0)

find_package(ROOT REQUIRED COMPONENTS Core)

# Define a macro to handle FetchContent_Declare with conditional DOWNLOAD_EXTRACT_TIMESTAMP
# This allows us to work with several versions of cmake.
include(FetchContent)
macro(FetchContent_Package name url)
  if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
    FetchContent_Declare(
      ${name}
      URL ${url}
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
  else()
    FetchContent_Declare(
      ${name}
      URL ${url}
    )
  endif()
  FetchContent_MakeAvailable(${name})
endmacro()

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Fetch our libraries
FetchContent_Package(googletest https://github.com/google/googletest/archive/075810f7a20405ea09a93f68847d6e963212fa62.zip)
FetchContent_Package(yaml-cpp https://github.com/jbeder/yaml-cpp/archive/refs/tags/0.8.0.zip)
FetchContent_Package(argparse https://github.com/p-ranav/argparse/archive/refs/tags/v3.2.zip)

if($ENV{AnalysisBase_VERSION} VERSION_LESS "25.0.0")
  find_package(Boost COMPONENTS program_options REQUIRED)
endif()

# Package setup
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# One main library
add_library(wraper_generators
            src/normalize.cpp
            src/class_info.cpp
            src/type_helpers.cpp
            src/utils.cpp
            src/util_string.cpp
            src/xaod_helpers.cpp
            src/helper_files.cpp
            src/metadata_file_finder.cpp
            src/translate.cpp
            )
target_link_libraries(wraper_generators ROOT::Core yaml-cpp)

# Executables for running the translation
add_executable(generate_types bin/generate_types.cpp)
target_link_libraries(generate_types ROOT::Core wraper_generators argparse stdc++fs)

# Tests
#
# ROOT's own externals stack (LCG) ships its own bundled gtest headers,
# and find_package(ROOT) puts that include directory ahead of our fetched
# googletest's on the search path (via wraper_generators -> ROOT::Core).
# That mismatches our test .cpp files (compiled against LCG's newer gtest
# headers) against the gtest library we actually build and link (from our
# pinned, older googletest commit), causing undefined references to
# MakeAndRegisterTestInfo at link time. Force our fetched googletest's
# include directory ahead of everything else so headers and library agree.
enable_testing()
set(func_adl_gtest_include_dir ${googletest_SOURCE_DIR}/googletest/include)

add_executable(t_type_helpers tests/t_type_helpers.cpp)
target_include_directories(t_type_helpers SYSTEM BEFORE PRIVATE ${func_adl_gtest_include_dir})
target_link_libraries(t_type_helpers wraper_generators gtest_main gtest)
add_executable(t_xaod_helpers tests/t_xaod_helpers.cpp)
target_include_directories(t_xaod_helpers SYSTEM BEFORE PRIVATE ${func_adl_gtest_include_dir})
target_link_libraries(t_xaod_helpers wraper_generators gtest_main gtest)
add_executable(t_class_info tests/t_class_info.cpp)
target_include_directories(t_class_info SYSTEM BEFORE PRIVATE ${func_adl_gtest_include_dir})
target_link_libraries(t_class_info wraper_generators gtest_main gtest)
add_executable(t_translate tests/t_translate.cpp)
target_include_directories(t_translate SYSTEM BEFORE PRIVATE ${func_adl_gtest_include_dir})
target_link_libraries(t_translate wraper_generators gtest_main gtest)
add_executable(t_metadata_file_finder tests/t_metadata_file_finder.cpp)
target_include_directories(t_metadata_file_finder SYSTEM BEFORE PRIVATE ${func_adl_gtest_include_dir})
target_link_libraries(t_metadata_file_finder wraper_generators gtest_main gtest stdc++fs)

include(GoogleTest)
gtest_discover_tests(t_type_helpers)
gtest_discover_tests(t_xaod_helpers)
gtest_discover_tests(t_class_info)
gtest_discover_tests(t_translate)
gtest_discover_tests(t_metadata_file_finder)
