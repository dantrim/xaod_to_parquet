# Copyright (C) 2002-2019 CERN for the benefit of the ATLAS collaboration
#
# This is a template for a CMakeLists.txt file that can be used in a client
# project (work area) to set up building ATLAS packages against the configured
# release.
#

# Set the minimum required CMake version:
cmake_minimum_required( VERSION 3.6 FATAL_ERROR )

# Make sure that all _ROOT variables *are* used when they are set.
if( POLICY CMP0074 )
   cmake_policy( SET CMP0074 NEW )
endif()

# If there's a directory called AtlasCMake in the project,
# and the user didn't specify AtlasCMake_DIR yet, then let's
# give it a default value.
if( IS_DIRECTORY ${CMAKE_SOURCE_DIR}/Build/AtlasCMake AND
      NOT AtlasCMake_DIR AND NOT ENV{AtlasCMake_DIR} )
   set( AtlasCMake_DIR ${CMAKE_SOURCE_DIR}/Build/AtlasCMake )
endif()

# If there's a directory called AtlasLCG in the project,
# and the user didn't specify LCG_DIR yet, then let's
# give it a default value.
if( IS_DIRECTORY ${CMAKE_SOURCE_DIR}/Build/AtlasLCG AND
      NOT LCG_DIR AND NOT ENV{LCG_DIR} )
   set( LCG_DIR ${CMAKE_SOURCE_DIR}/Build/AtlasLCG )
endif()

# Pick up a local version of the AtlasCMake code if it exists:
find_package( AtlasCMake QUIET )

# Find the project that we depend on:
find_package( AnalysisBase )

# Set up CTest:
atlas_ctest_setup()

#
# externals
#
add_subdirectory(parquet-writer)
set(PARQUET_WRITER_INCLUDE_DIR parquet-writer/src/cpp)

# Set up a work directory project:
set(ANALYSIS_BASE_VERSION $ENV{AnalysisBaseExternals_VERSION})
atlas_project( WorkDir ${ANALYSIS_BASE_VERSION}
    USE AnalysisBase ${ANALYSIS_BASE_VERSION}
    )

# Set up the runtime environment setup script(s):
lcg_generate_env( SH_FILE ${CMAKE_BINARY_DIR}/${ATLAS_PLATFORM}/env_setup.sh )
install( FILES ${CMAKE_BINARY_DIR}/${ATLAS_PLATFORM}/env_setup.sh
   DESTINATION . )

# Set up CPack:
atlas_cpack_setup()
