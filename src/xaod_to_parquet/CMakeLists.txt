# the name of this project
set(ATLAS_PROJECT_NAME xaod_to_parqet)
atlas_subdir(${ATLAS_PROJECT_NAME})

set(extra_deps)
set(extra_libs)

##
## AnalysisBase
##
atlas_depends_on_subdirs(
    PUBLIC
    Control/xAODRootAccess
    Control/AthToolSupport/AsgTools
    Event/xAOD/xAODCore
    Event/xAOD/xAODCutFlow
    Event/xAOD/xAODPrimitives
    Event/xAOD/xAODMetaData
    Tools/PathResolver
    Event/xAOD/xAODJet
    Event/xAOD/xAODEgamma
    Event/xAOD/xAODMuon
    Event/xAOD/xAODMissingET
    Trigger/TrigAnalysis/TrigDecisionTool
    Trigger/TrigConfiguration/TrigConfxAOD
    PhysicsAnalysis/SUSYPhys/SUSYTools
    PRIVATE
    ${extra_deps}
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

atlas_add_library(x2p-analysis
    ${ATLAS_PROJECT_NAME}/*.h
    src/*.cpp
    PUBLIC_HEADERS ${ATLAS_PROJECT_NAME}
    PRIVATE_INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
    LINK_LIBRARIES
    SUSYToolsLib
    AsgTools
    xAODCore
    xAODPrimitives
    xAODCutFlow
    xAODRootAccess
    xAODJet
    TrigDecisionToolLib
    TrigConfxAODLib
    PathResolver
    xAODMetaData
    PRIVATE_LINK_LIBRARIES
    ${ROOT_LIBRARIES}
)

set(extra_libs)

##
## executables
##
atlas_add_executable(dump-parquet util/dump-parquet.cpp
    INCLUDE_DIRS ${INCLUDES} LINK_LIBRARIES ${LIBRARIES}
)

function(X2PFunc filename)
    set(execname)
    get_filename_component(execname ${filename} NAME_WE)
    atlas_add_executable(${execname} "util/${execname}.cpp"
        INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
        LINK_LIBRARIES ${ROOT_LIBRARIES} x2p-analysis ${extra_libs}
    )
endfunction(X2PFunc)
file(GLOB files "util/*.cpp")
foreach(file ${files})
    X2PFunc(${file})
endforeach()

atlas_install_data(data/*)
