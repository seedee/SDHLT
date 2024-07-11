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

message("${CMAKE_PROJECT_NAME} > Tools will use prefix \"${SDHLT_PREFIX}${SDHLT_GAME_PREFIX}\"")

set(SDHLT_DIR ${CMAKE_SOURCE_DIR}/src/sdhlt)

#================
# Common
#================

set(COMMON_DIR ${SDHLT_DIR}/common)
set(TEMPLATE_DIR ${SDHLT_DIR}/template)

set(COMMON_SOURCES
    ${COMMON_DIR}/blockmem.cpp
    ${COMMON_DIR}/bspfile.cpp
    ${COMMON_DIR}/cmdlib.cpp
    ${COMMON_DIR}/cmdlinecfg.cpp
    ${COMMON_DIR}/filelib.cpp
    ${COMMON_DIR}/log.cpp
    ${COMMON_DIR}/mathlib.cpp
    ${COMMON_DIR}/messages.cpp
    ${COMMON_DIR}/scriplib.cpp
    ${COMMON_DIR}/threads.cpp
    ${COMMON_DIR}/winding.cpp
)

set(COMMON_HEADERS
    ${TEMPLATE_DIR}/basictypes.h
    ${COMMON_DIR}/blockmem.h
    ${COMMON_DIR}/boundingbox.h
    ${COMMON_DIR}/bspfile.h
    ${COMMON_DIR}/cmdlib.h
    ${COMMON_DIR}/cmdlinecfg.h
    ${COMMON_DIR}/filelib.h
    ${COMMON_DIR}/hlassert.h
    ${COMMON_DIR}/log.h
    ${COMMON_DIR}/mathlib.h
    ${COMMON_DIR}/mathtypes.h
    ${COMMON_DIR}/messages.h
    ${COMMON_DIR}/scriplib.h
    ${COMMON_DIR}/threads.h
    ${COMMON_DIR}/win32fix.h
    ${COMMON_DIR}/winding.h
)

#================
# BSP
#================

set(BSP_DIR ${SDHLT_DIR}/sdHLBSP)

set(BSP_SOURCES
    ${COMMON_SOURCES}
    ${BSP_DIR}/brink.cpp
    ${BSP_DIR}/merge.cpp
    ${BSP_DIR}/outside.cpp
    ${BSP_DIR}/portals.cpp
    ${BSP_DIR}/qbsp.cpp
    ${BSP_DIR}/solidbsp.cpp
    ${BSP_DIR}/surfaces.cpp
    ${BSP_DIR}/tjunc.cpp
    ${BSP_DIR}/writebsp.cpp
)

set(BSP_HEADERS
    ${COMMON_HEADERS}
    ${BSP_DIR}/bsp5.h
)

#================
# CSG
#================

set(CSG_DIR ${SDHLT_DIR}/sdHLCSG)

set(CSG_SOURCES
    ${COMMON_SOURCES}
    ${CSG_DIR}/ansitoutf8.cpp
    ${CSG_DIR}/brush.cpp
    ${CSG_DIR}/brushunion.cpp
    ${CSG_DIR}/hullfile.cpp
    ${CSG_DIR}/map.cpp
    ${CSG_DIR}/properties.cpp
    ${CSG_DIR}/qcsg.cpp
    ${CSG_DIR}/textures.cpp
    ${CSG_DIR}/wadcfg.cpp
    ${CSG_DIR}/wadpath.cpp
)

set(CSG_HEADERS
    ${COMMON_HEADERS}
    ${CSG_DIR}/csg.h
    ${CSG_DIR}/wadpath.h
)

#================
# RAD
#================

set(RAD_DIR ${SDHLT_DIR}/sdHLRAD)

set(RAD_SOURCES
    ${COMMON_SOURCES}
    ${RAD_DIR}/compress.cpp
    ${RAD_DIR}/lerp.cpp
    ${RAD_DIR}/lightmap.cpp
    ${RAD_DIR}/loadtextures.cpp
    ${RAD_DIR}/mathutil.cpp
	${RAD_DIR}/meshdesc.cpp
	${RAD_DIR}/meshtrace.cpp
    ${RAD_DIR}/nomatrix.cpp
    ${RAD_DIR}/progmesh.cpp
    ${RAD_DIR}/qrad.cpp
    ${RAD_DIR}/qradutil.cpp
    ${RAD_DIR}/sparse.cpp
	${RAD_DIR}/stringlib.cpp
	${RAD_DIR}/studio.cpp
    ${RAD_DIR}/trace.cpp
    ${RAD_DIR}/transfers.cpp
    ${RAD_DIR}/transparency.cpp
    ${RAD_DIR}/vismatrix.cpp
    ${RAD_DIR}/vismatrixutil.cpp
)

set(RAD_HEADERS
    ${COMMON_HEADERS}
    ${COMMON_DIR}/anorms.h
    ${RAD_DIR}/compress.h
	${RAD_DIR}/list.h
	${RAD_DIR}/meshdesc.h
	${RAD_DIR}/meshtrace.h
    ${RAD_DIR}/qrad.h
)

#================
# VIS
#================

set(VIS_DIR ${SDHLT_DIR}/sdHLVIS)

set(VIS_SOURCES
    ${COMMON_SOURCES}
    ${VIS_DIR}/flow.cpp
    ${VIS_DIR}/vis.cpp
    ${VIS_DIR}/zones.cpp
)

set(VIS_HEADERS
    ${COMMON_HEADERS}
    ${VIS_DIR}/vis.h
    ${VIS_DIR}/zones.h
)

#================
# RIPENT
#================

set(RIPENT_DIR ${SDHLT_DIR}/sdRIPENT)

set(RIPENT_SOURCES
    ${COMMON_SOURCES}
    ${RIPENT_DIR}/ripent.cpp
)

set(RIPENT_HEADERS
    ${COMMON_HEADERS}
    ${RIPENT_DIR}/ripent.h
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
    )
    add_compile_definitions(
        VERSION_LINUX
        SYSTEM_POSIX
        STDC_HEADERS
        HAVE_FCNTL_H
        HAVE_PTHREAD_H
        HAVE_STDDEF_H
        HAVE_SYS_RESOURCE_H
        HAVE_SYS_STAT_H
        HAVE_SYS_TIME_H
        HAVE_UNISTD_H
    )
    message("${CMAKE_PROJECT_NAME} > Configuring for ${CMAKE_SYSTEM_NAME}")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    option(SDHLT_64BIT "Flag targets as 64-bit." ON)
    if (SDHLT_64BIT)
        add_compile_definitions(VERSION_64BIT)
    else()
        add_compile_definitions(VERSION_32BIT)
    endif()
    add_compile_definitions(
        SYSTEM_WIN32
        STDC_HEADERS
    )
    message("${CMAKE_PROJECT_NAME} > Configuring for ${CMAKE_SYSTEM_NAME}")
else()
    add_compile_definitions(VERSION_OTHER)
    message("${CMAKE_PROJECT_NAME} > WARNING: Unknown system \"${CMAKE_SYSTEM_NAME}\"")
endif()

#================
# Targets
#================

add_executable(BSP ${BSP_SOURCES} ${BSP_HEADERS})
add_executable(CSG ${CSG_SOURCES} ${CSG_HEADERS})
add_executable(RAD ${RAD_SOURCES} ${RAD_HEADERS})
add_executable(VIS ${VIS_SOURCES} ${VIS_HEADERS})
add_executable(RIPENT ${RIPENT_SOURCES} ${RIPENT_HEADERS})

set_target_properties(BSP CSG RAD VIS
    PROPERTIES
        PREFIX ${SDHLT_PREFIX}${SDHLT_GAME_PREFIX}
)
set_target_properties(RIPENT
    PROPERTIES
        PREFIX ${SDHLT_PREFIX}
)

target_compile_definitions(BSP PRIVATE SDHLBSP DOUBLEVEC_T)
target_compile_definitions(CSG PRIVATE SDHLCSG DOUBLEVEC_T)
target_compile_definitions(RAD PRIVATE SDHLRAD)
target_compile_definitions(VIS PRIVATE SDHLVIS)
target_compile_definitions(RIPENT PRIVATE SDRIPENT)
