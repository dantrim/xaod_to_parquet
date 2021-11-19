# the name of this project
set(ATLAS_PROJECT_NAME xaod_to_parqet)
atlas_subdir(xaod_to_parquet)

##
## AnalysisBase
##
atlas_depends_on_subdirs(
    PRIVATE
    Control/xAODRootAccess
    Event/xAOD/xAODCore
)

find_package(ROOT REQUIRED COMPONENTS RIO Tree Core)

set(INCLUDES
    ${ATLAS_PROJECT_NAME}/*.h
    ${ATLAS_PROJECT_NAME}/*.hpp
    ${ROOT_INCLUDE_DIRS}
    ${PARQUET_WRITER_INCLUDE_DIR}/*.h
)

set(LIBRARIES
    ${ROOT_LIBRARIES}
    xAODRootAccess
    parquet-writer
)

##
## executables
##
atlas_add_executable(dump-parquet util/dump-parquet.cpp
    INCLUDE_DIRS ${INCLUDES} LINK_LIBRARIES ${LIBRARIES}
)
