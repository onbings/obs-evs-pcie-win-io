#Windows build
#cmake -DCMAKE_BUILD_TYPE=Release -Dlibobs_DIR="E:/pro/obs-studio/build_x64/libobs" -DCMAKE_TOOLCHAIN_FILE=C:/pro/vcpkg/scripts/buildsystems/vcpkg.cmake --preset windows-x64
#cmake --build build --config Release

cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" OFF)

# Root for the external build artifacts (contains src/, Debug/, Release/)
set(EVS_PCIE_WIN_IO_ROOT "C:/bld/evs-pcie-win-io/package/api" CACHE PATH "Root directory of evs-pcie-win-io (contains src/, Debug/, Release/)")
set(EVS_PCIE_WIN_IO_INCLUDE_DIR "${EVS_PCIE_WIN_IO_ROOT}" CACHE PATH "Path to evs-pcie-win-io include directory")
set(EVS_PCIE_WIN_IO_LIB_DEBUG   "${EVS_PCIE_WIN_IO_ROOT}/Debug/libevs-pcie-win-io-api.lib" CACHE FILEPATH "Debug import library for evs-pcie-win-io")
set(EVS_PCIE_WIN_IO_LIB_RELEASE "${EVS_PCIE_WIN_IO_ROOT}/Release/libevs-pcie-win-io-api.lib" CACHE FILEPATH "Release import library for evs-pcie-win-io")

# Root rundir for OBS Studio runtime copies; final path will append configuration and plugin subpath
set(OBS_STUDIO_RUNDIR_ROOT "E:/pro/obs-studio/build_x64/rundir" CACHE PATH "Root rundir where OBS build places runtimes (plugin path will be <ROOT>/$<CONFIG>/obs-plugins/64bit)")

include(compilerconfig)
include(defaults)
include(helpers)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/obs-evs-pcie-win-io-dependencies.cmake
  @ONLY
)

include(${CMAKE_CURRENT_BINARY_DIR}/obs-evs-pcie-win-io-dependencies.cmake)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

set(ENABLE_FRONTEND_API ON)
message("=========ENABLE_FRONTEND_API=========> " ${ENABLE_FRONTEND_API})
if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  message("==================> obs-frontend-api")
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

add_library(libevs_pcie_win_io_api STATIC IMPORTED)
set_target_properties(libevs_pcie_win_io_api PROPERTIES
IMPORTED_LOCATION_DEBUG  "${EVS_PCIE_WIN_IO_LIB_DEBUG}"
IMPORTED_LOCATION_RELEASE "${EVS_PCIE_WIN_IO_LIB_RELEASE}"
INTERFACE_INCLUDE_DIRECTORIES "${EVS_PCIE_WIN_IO_INCLUDE_DIR}"
)
target_link_libraries(${CMAKE_PROJECT_NAME} 
PRIVATE 
 libevs_pcie_win_io_api
 PThreads4W::PThreads4W
 nlohmann_json::nlohmann_json
 vectorclass::vectorclass
 ONBINGS::bofstd
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
"${EVS_PCIE_WIN_IO_INCLUDE_DIR}"
)
target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_20)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE 
src/plugin-main.cpp
src/BoardResourceManager.h
src/BoardResourceManager.cpp
src/SimpleRecorder.h
src/SimpleRecorder.cpp
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

# Post-build step to copy plugin to OBS Studio build directory
# Uses generator expression $<CONFIG> so the correct configuration folder (Debug/Release) is used.
#add_custom_command(
#  TARGET ${CMAKE_PROJECT_NAME}
#  POST_BUILD
#  COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
#    "${OBS_STUDIO_RUNDIR_ROOT}/$<CONFIG>/obs-plugins/64bit/$<TARGET_FILE_NAME:${CMAKE_PROJECT_NAME}>"
#  COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/$<TARGET_FILE_BASE_NAME:${CMAKE_PROJECT_NAME}>.pdb"
#    "${OBS_STUDIO_RUNDIR_ROOT}/$<CONFIG>/obs-plugins/64bit/$<TARGET_FILE_BASE_NAME:${CMAKE_PROJECT_NAME}>.pdb"
#  COMMENT "Copying plugin to OBS Studio build directory"
#)

set(OBS_DIR "E:/pro/obs-studio")
set(PLUGIN_DIR "E:/pro/obs-evs-pcie-win-io")
add_custom_command(
  TARGET ${CMAKE_PROJECT_NAME}
  POST_BUILD

  COMMAND "C:/Program Files/PowerShell/7/pwsh.exe"
    -noprofile -executionpolicy Bypass
    -file C:/pro/vcpkg/scripts/buildsystems/msbuild/applocal.ps1
    -targetBinary "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
    -installedDir "${PLUGIN_DIR}/build_x64/vcpkg_installed/x64-windows/$<IF:$<CONFIG:Debug>,debug,release>/bin"
    -OutVariable out

  COMMAND ${CMAKE_COMMAND} -E make_directory
    "${OBS_STUDIO_RUNDIR_ROOT}/$<CONFIG>"

  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
    "$<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}>"
    "${OBS_STUDIO_RUNDIR_ROOT}/$<CONFIG>"

  COMMAND ${CMAKE_COMMAND} -E make_directory
    "${OBS_STUDIO_RUNDIR_ROOT}/$<CONFIG>/data/obs-plugins/obs-evs-pcie-win-io/locale"

  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PLUGIN_DIR}/data/locale"
    "${OBS_STUDIO_RUNDIR_ROOT}/$<CONFIG>/data/obs-plugins/obs-evs-pcie-win-io/locale"

  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
    "${OBS_STUDIO_RUNDIR_ROOT}/$<CONFIG>/obs-plugins/64bit/$<TARGET_FILE_NAME:${CMAKE_PROJECT_NAME}>"

  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}>"
    "${OBS_STUDIO_RUNDIR_ROOT}/$<CONFIG>/obs-plugins/64bit/$<TARGET_FILE_BASE_NAME:${CMAKE_PROJECT_NAME}>.pdb"

  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PLUGIN_DIR}/data/obs-evs-pcie-win-io-config.json"
    "${OBS_STUDIO_RUNDIR_ROOT}/$<CONFIG>/obs-plugins/64bit/obs-evs-pcie-win-io-config.json"

  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PLUGIN_DIR}/build_x64/$<CONFIG>/pthreadVC3d.dll"
    "${OBS_STUDIO_RUNDIR_ROOT}/$<CONFIG>/bin/64bit/pthreadVC3d.dll"

  COMMAND ${CMAKE_COMMAND} -E echo "Building with configuration:"
  COMMAND ${CMAKE_COMMAND} -E echo "  Source: ${PLUGIN_DIR}/build_x64/$<CONFIG>/$<IF:$<CONFIG:Debug>,pthreadVC3d.dll,pthreadVC3.dll>"
  COMMAND ${CMAKE_COMMAND} -E echo "  Dest: ${OBS_STUDIO_RUNDIR_ROOT}/$<CONFIG>/bin/64bit/$<IF:$<CONFIG:Debug>,pthreadVC3d.dll,pthreadVC3.dll>"

  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PLUGIN_DIR}/build_x64/$<CONFIG>/$<IF:$<CONFIG:Debug>,pthreadVC3d.dll,pthreadVC3.dll>"
    "${OBS_STUDIO_RUNDIR_ROOT}/$<CONFIG>/bin/64bit/$<IF:$<CONFIG:Debug>,pthreadVC3d.dll,pthreadVC3.dll>"

  COMMENT "Post-build: applocal, copy DLL/PDB, copy locale, copy obs-evs-pcie-win-io-config.json"
)
