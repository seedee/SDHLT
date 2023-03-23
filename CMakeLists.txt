#================================
# SDHLT
#================================

cmake_minimum_required(VERSION 3.20)

project(sdhlt
    DESCRIPTION "seedee's Half-Life Compilation Tools"
    LANGUAGES CXX
)

message("${CMAKE_PROJECT_NAME} > Starting configuration for ${CMAKE_PROJECT_DESCRIPTION}")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/tools)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/tools)

#================
# Configuration
#================

set(SDHLT_PREFIX "sd" CACHE STRING "Executable prefix.")
set(SDHLT_GAME_PREFIX "HL" CACHE STRING "Executable game prefix.")
option(SDHLT_32BIT "Generate 32-bit targets." OFF)
option(SDHLT_64BIT "Generate 64-bit targets." ON)

message("${CMAKE_PROJECT_NAME} > Tools will use prefix \"${SDHLT_PREFIX}${SDHLT_GAME_PREFIX}\"")

set(SDHLT_DIR ${CMAKE_SOURCE_DIR}/src/sdhlt)

#================
# BSP
#================

set(BSP_SOURCES
    ${SDHLT_DIR}/common/blockmem.cpp
    ${SDHLT_DIR}/common/bspfile.cpp
    ${SDHLT_DIR}/common/cmdlib.cpp
    ${SDHLT_DIR}/common/cmdlinecfg.cpp
    ${SDHLT_DIR}/common/filelib.cpp
    ${SDHLT_DIR}/common/log.cpp
    ${SDHLT_DIR}/common/mathlib.cpp
    ${SDHLT_DIR}/common/messages.cpp
    ${SDHLT_DIR}/common/scriplib.cpp
    ${SDHLT_DIR}/common/threads.cpp
    ${SDHLT_DIR}/common/winding.cpp
    ${SDHLT_DIR}/sdHLBSP/brink.cpp
    ${SDHLT_DIR}/sdHLBSP/merge.cpp
    ${SDHLT_DIR}/sdHLBSP/outside.cpp
    ${SDHLT_DIR}/sdHLBSP/portals.cpp
    ${SDHLT_DIR}/sdHLBSP/qbsp.cpp
    ${SDHLT_DIR}/sdHLBSP/solidbsp.cpp
    ${SDHLT_DIR}/sdHLBSP/surfaces.cpp
    ${SDHLT_DIR}/sdHLBSP/tjunc.cpp
    ${SDHLT_DIR}/sdHLBSP/writebsp.cpp
)

#================
# CSG
#================

set(CSG_SOURCES
    ${SDHLT_DIR}/common/blockmem.cpp
    ${SDHLT_DIR}/common/bspfile.cpp
    ${SDHLT_DIR}/common/cmdlib.cpp
    ${SDHLT_DIR}/common/cmdlinecfg.cpp
    ${SDHLT_DIR}/common/filelib.cpp
    ${SDHLT_DIR}/common/log.cpp
    ${SDHLT_DIR}/common/mathlib.cpp
    ${SDHLT_DIR}/common/messages.cpp
    ${SDHLT_DIR}/common/scriplib.cpp
    ${SDHLT_DIR}/common/threads.cpp
    ${SDHLT_DIR}/common/winding.cpp
    ${SDHLT_DIR}/sdHLCSG/ansitoutf8.cpp
    ${SDHLT_DIR}/sdHLCSG/autowad.cpp
    ${SDHLT_DIR}/sdHLCSG/brush.cpp
    ${SDHLT_DIR}/sdHLCSG/brushunion.cpp
    ${SDHLT_DIR}/sdHLCSG/hullfile.cpp
    ${SDHLT_DIR}/sdHLCSG/map.cpp
    ${SDHLT_DIR}/sdHLCSG/properties.cpp
    ${SDHLT_DIR}/sdHLCSG/qcsg.cpp
    ${SDHLT_DIR}/sdHLCSG/textures.cpp
    ${SDHLT_DIR}/sdHLCSG/wadcfg.cpp
    ${SDHLT_DIR}/sdHLCSG/wadinclude.cpp
    ${SDHLT_DIR}/sdHLCSG/wadpath.cpp
)

#================
# RAD
#================

set(RAD_SOURCES
    ${SDHLT_DIR}/common/blockmem.cpp
    ${SDHLT_DIR}/common/bspfile.cpp
    ${SDHLT_DIR}/common/cmdlib.cpp
    ${SDHLT_DIR}/common/cmdlinecfg.cpp
    ${SDHLT_DIR}/common/filelib.cpp
    ${SDHLT_DIR}/common/log.cpp
    ${SDHLT_DIR}/common/mathlib.cpp
    ${SDHLT_DIR}/common/messages.cpp
    ${SDHLT_DIR}/common/scriplib.cpp
    ${SDHLT_DIR}/common/threads.cpp
    ${SDHLT_DIR}/common/winding.cpp
    ${SDHLT_DIR}/sdHLRAD/compress.cpp
    ${SDHLT_DIR}/sdHLRAD/lerp.cpp
    ${SDHLT_DIR}/sdHLRAD/lightmap.cpp
    ${SDHLT_DIR}/sdHLRAD/loadtextures.cpp
    ${SDHLT_DIR}/sdHLRAD/mathutil.cpp
    ${SDHLT_DIR}/sdHLRAD/nomatrix.cpp
    ${SDHLT_DIR}/sdHLRAD/qrad.cpp
    ${SDHLT_DIR}/sdHLRAD/qradutil.cpp
    ${SDHLT_DIR}/sdHLRAD/sparse.cpp
    ${SDHLT_DIR}/sdHLRAD/trace.cpp
    ${SDHLT_DIR}/sdHLRAD/transfers.cpp
    ${SDHLT_DIR}/sdHLRAD/transparency.cpp
    ${SDHLT_DIR}/sdHLRAD/vismatrix.cpp
    ${SDHLT_DIR}/sdHLRAD/vismatrixutil.cpp
)

#================
# VIS
#================

set(VIS_SOURCES
    ${SDHLT_DIR}/common/blockmem.cpp
    ${SDHLT_DIR}/common/bspfile.cpp
    ${SDHLT_DIR}/common/cmdlib.cpp
    ${SDHLT_DIR}/common/cmdlinecfg.cpp
    ${SDHLT_DIR}/common/filelib.cpp
    ${SDHLT_DIR}/common/log.cpp
    ${SDHLT_DIR}/common/mathlib.cpp
    ${SDHLT_DIR}/common/messages.cpp
    ${SDHLT_DIR}/common/scriplib.cpp
    ${SDHLT_DIR}/common/threads.cpp
    ${SDHLT_DIR}/common/winding.cpp
    ${SDHLT_DIR}/sdHLVIS/flow.cpp
    ${SDHLT_DIR}/sdHLVIS/vis.cpp
    ${SDHLT_DIR}/sdHLVIS/zones.cpp
)

#================
# RIPENT
#================

set(RIPENT_SOURCES
    ${SDHLT_DIR}/common/blockmem.cpp
    ${SDHLT_DIR}/common/bspfile.cpp
    ${SDHLT_DIR}/common/cmdlib.cpp
    ${SDHLT_DIR}/common/cmdlinecfg.cpp
    ${SDHLT_DIR}/common/filelib.cpp
    ${SDHLT_DIR}/common/files.cpp
    ${SDHLT_DIR}/common/log.cpp
    ${SDHLT_DIR}/common/mathlib.cpp
    ${SDHLT_DIR}/common/messages.cpp
    ${SDHLT_DIR}/common/scriplib.cpp
    ${SDHLT_DIR}/common/threads.cpp
    ${SDHLT_DIR}/common/winding.cpp
    ${SDHLT_DIR}/sdRIPENT/ripent.cpp
)

#================
# Include
#================

include_directories(
    ${SDHLT_DIR}/common
    ${SDHLT_DIR}/template
)

#================
# System config
#================

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_compile_options(
        -Wall
        -O2
        -fno-strict-aliasing
        -pthread
        -pipe
        -DSYSTEM_POSIX
        -DSTDC_HEADERS
        -DHAVE_FCNTL_H
        -DHAVE_PTHREAD_H
        -DHAVE_STDDEF_H
        -DHAVE_SYS_RESOURCE_H
        -DHAVE_SYS_STAT_H
        -DHAVE_SYS_TIME_H
        -DHAVE_UNISTD_H
    )
    message("${CMAKE_PROJECT_NAME} > Configuring for ${CMAKE_SYSTEM_NAME}")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_compile_options(
        -DSYSTEM_WIN32
        -DSTDC_HEADERS
    )
    message("${CMAKE_PROJECT_NAME} > Configuring for ${CMAKE_SYSTEM_NAME}")
else()
    message("${CMAKE_PROJECT_NAME} > WARNING: Unknown system \"${CMAKE_SYSTEM_NAME}\"")
endif()

#================
# 32-bit targets
#================

if (SDHLT_32BIT)
    message("${CMAKE_PROJECT_NAME} > Generating 32-bit targets")

    add_executable(BSP ${BSP_SOURCES})
    add_executable(CSG ${CSG_SOURCES})
    add_executable(RAD ${RAD_SOURCES})
    add_executable(VIS ${VIS_SOURCES})
    add_executable(RIPENT ${RIPENT_SOURCES})

    set_target_properties(BSP CSG RAD VIS
        PROPERTIES
            PREFIX ${SDHLT_PREFIX}${SDHLT_GAME_PREFIX}
    )
    set_target_properties(RIPENT
        PROPERTIES
            PREFIX ${SDHLT_PREFIX}
    )
    
    target_compile_options(BSP PRIVATE -m32 -DVERSION_32BIT=1 -DSDHLBSP -DDOUBLEVEC_T)
    target_compile_options(CSG PRIVATE -m32 -DVERSION_32BIT=1 -DSDHLCSG -DDOUBLEVEC_T)
    target_compile_options(RAD PRIVATE -m32 -DVERSION_32BIT=1 -DSDHLRAD)
    target_compile_options(VIS PRIVATE -m32 -DVERSION_32BIT=1 -DSDHLVIS)
    target_compile_options(RIPENT PRIVATE -m32 -DVERSION_32BIT=1 -DSDRIPENT)
endif()

#================
# 64-bit targets
#================

if (SDHLT_64BIT)
    message("${CMAKE_PROJECT_NAME} > Generating 64-bit targets")

    add_executable(BSP_x64 ${BSP_SOURCES})
    add_executable(CSG_x64 ${CSG_SOURCES})
    add_executable(RAD_x64 ${RAD_SOURCES})
    add_executable(VIS_x64 ${VIS_SOURCES})
    add_executable(RIPENT_x64 ${RIPENT_SOURCES})

    set_target_properties(BSP_x64 CSG_x64 RAD_x64 VIS_x64
        PROPERTIES
            PREFIX ${SDHLT_PREFIX}${SDHLT_GAME_PREFIX}
    )
    set_target_properties(RIPENT_x64
        PROPERTIES
            PREFIX ${SDHLT_PREFIX}
    )

    target_compile_options(BSP_x64 PRIVATE -m64 -DVERSION_64BIT=1 -DSDHLBSP -DDOUBLEVEC_T)
    target_compile_options(CSG_x64 PRIVATE -m64 -DVERSION_64BIT=1 -DSDHLCSG -DDOUBLEVEC_T)
    target_compile_options(RAD_x64 PRIVATE -m64 -DVERSION_64BIT=1 -DSDHLRAD)
    target_compile_options(VIS_x64 PRIVATE -m64 -DVERSION_64BIT=1 -DSDHLVIS)
    target_compile_options(RIPENT_x64 PRIVATE -m64 -DVERSION_64BIT=1 -DSDRIPENT)
endif()
