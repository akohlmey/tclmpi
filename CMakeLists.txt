# CMake file for the TclMPI Tcl wrapper around MPI

cmake_minimum_required(VERSION 3.16)
project(TclMPI VERSION 1.1 DESCRIPTION "Tcl Bindings for MPI" LANGUAGES C)

# compilation settings and options
option(ENABLE_TCL_STUBS "Build TclMPI using the Tcl stub library" ON)
option(BUILD_TCLMPI_SHELL "Build Tcl interpreter with TclMPI embedded" ON)
# this is used for automated compiles and cross-compilation
option(DOWNLOAD_MPICH4WIN "Download and use MPICH-1.4.1 on Windows" OFF)
mark_as_advanced(TTK_STUB_LIBRARY)

# tweaks
set(BUILD_SHARED_LIBS TRUE)
set(MPI_CXX_SKIP_MPICXX TRUE)

# find packages
if((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND DOWNLOAD_MPICH4WIN)
  message(STATUS "Downloading and configuring MPICH-1.4.1 for Windows")
  include(ExternalProject)
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    ExternalProject_Add(mpi4win_build
      URL     https://download.lammps.org/thirdparty/mpich2-win64-devel.tar.gz
      URL_MD5 4939fdb59d13182fd5dd65211e469f14
      CONFIGURE_COMMAND "" BUILD_COMMAND "" INSTALL_COMMAND ""
      BUILD_BYPRODUCTS <SOURCE_DIR>/lib/libmpi.a)
  else()
    message(FATAL_ERROR "Unsupported Windows platform ${CMAKE_SYSTEM_PROCESSOR}")
  endif()
  ExternalProject_get_property(mpi4win_build SOURCE_DIR)
  file(MAKE_DIRECTORY "${SOURCE_DIR}/include")
  add_library(MPI::MPI_C UNKNOWN IMPORTED)
  set_target_properties(MPI::MPI_C PROPERTIES
    IMPORTED_LOCATION "${SOURCE_DIR}/lib/libmpi.a"
    INTERFACE_INCLUDE_DIRECTORIES "${SOURCE_DIR}/include"
    INTERFACE_COMPILE_DEFINITIONS "MPICH_SKIP_MPICXX")
  add_dependencies(MPI::MPI_C mpi4win_build)
else()
  find_package(MPI REQUIRED)
endif()
find_package(TCL)
if(!TCL_FOUND)
  message(FATAL_ERROR "Must have Tcl installed to build TclMPI")
endif()
if(ENABLE_TCL_STUBS)
  find_package(TclStub)
endif()
find_package(Doxygen OPTIONAL_COMPONENTS dot)
find_package(Git)

# extract Tcl version number from tcl.h
file(READ ${TCL_INCLUDE_PATH}/tcl.h _tcl_header)
string(REGEX MATCH "TCL_VERSION[ \t]*\"[0-9]\.[0-9]*\"" _ ${_tcl_header})
string(REGEX MATCH "[0-9]\.[0-9]*" TCL_VERSION ${_})

# build shared object
add_library(_tclmpi MODULE _tclmpi.c)
set_target_properties(_tclmpi PROPERTIES PREFIX "")
target_include_directories(_tclmpi PRIVATE ${TCL_INCLUDE_PATH})
target_compile_definitions(_tclmpi PRIVATE -DPACKAGE_NAME="_tclmpi" -DPACKAGE_VERSION="${CMAKE_PROJECT_VERSION}")
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  target_compile_definitions(_tclmpi PRIVATE -DMPIWRAPSTCLDLL_EXPORTS)
endif()
if(ENABLE_TCL_STUBS)
  target_compile_definitions(_tclmpi PRIVATE -DUSE_TCL_STUBS)
  target_link_libraries(_tclmpi PRIVATE ${TCL_STUB_LIBRARY})
else()
  target_link_libraries(_tclmpi PRIVATE ${TCL_LIBRARY})
endif()
target_link_libraries(_tclmpi PRIVATE MPI::MPI_C)

# build Tcl shell extended with MPI
if(BUILD_TCLMPI_SHELL)
  add_executable(tclmpish _tclmpi.c)
  target_include_directories(tclmpish PRIVATE ${TCL_INCLUDE_PATH})
  target_compile_definitions(tclmpish PRIVATE -DPACKAGE_NAME="_tclmpi" -DPACKAGE_VERSION="${CMAKE_PROJECT_VERSION}" -DBUILD_TCLMPISH)
  target_link_libraries(tclmpish PRIVATE ${TCL_LIBRARY})
  target_link_libraries(tclmpish PRIVATE MPI::MPI_C)
endif()

# embed version numbers and file names
set(TCLMPI_BINARY_MODULE _tclmpi${CMAKE_SHARED_LIBRARY_SUFFIX})
configure_file(tclmpi.tcl.in tclmpi.tcl @ONLY)
configure_file(pkgIndex.tcl.in pkgIndex.tcl @ONLY)
configure_file(tests/harness.tcl.in harness.tcl @ONLY)

# extract documentation from Tcl files
function(TclFileToDox infile outfile)
  message(STATUS "Extracting documentation from Tcl file ${infile}")
  file(READ ${infile} _one)
  # hide semicolons so we can loop over lines by replacing EOLs with a ;
  string(REGEX REPLACE ";" "#Y#" _two "${_one}")
  string(REGEX REPLACE "\\\n" ";" _three "${_two}")
  set(_all "")
  foreach(_line ${_three})
    # concatenate only lines with a #X# comment
    string(FIND "${_line}" "#X#" _found)
    if(_found GREATER -1)
      string(REGEX REPLACE "#X#" "" _four "${_line}")
      string(APPEND _all "${_four}\n")
    endif()
  endforeach()
  # put back the semicolons and write the result to file
  string(REGEX REPLACE "#Y#" ";" _out "${_all}")
  message(STATUS "Writing ${outfile}")
  file(WRITE ${outfile} "${_out}")
endfunction(TclFileToDox)
TclFileToDox(${CMAKE_BINARY_DIR}/tclmpi.tcl ${CMAKE_BINARY_DIR}/tclmpi.dox)
TclFileToDox(${CMAKE_BINARY_DIR}/harness.tcl ${CMAKE_BINARY_DIR}/harness.dox)

# build documentation
if(DOXYGEN_FOUND)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md")
  set(DOXYGEN_GENERATE_LATEX YES)
  set(DOXYGEN_FULL_PATH_NAMES NO)
  set(DOXYGEN_SHOW_FILES NO)
  set(DOXYGEN_HTML_COLORSTYLE_HUE 224)
  set(DOXYGEN_HTML_COLORSTYLE_SAT 137)
  set(DOXYGEN_HTML_COLORSTYLE_GAMMA 103)
  set(DOXYGEN_GENERATE_TREEVIEW YES)
  set(DOXYGEN_TREEVIEW_WIDTH 200)

  doxygen_add_docs(docs
    ${CMAKE_SOURCE_DIR}/README.md
    ${CMAKE_SOURCE_DIR}/_tclmpi.c
    ${CMAKE_BINARY_DIR}/tclmpi.dox
    ${CMAKE_BINARY_DIR}/tclmpi.tcl
    ${CMAKE_BINARY_DIR}/harness.dox
    ${CMAKE_BINARY_DIR}/harness.tcl)
  set_property(TARGET docs APPEND PROPERTY ADDITIONAL_CLEAN_FILES latex ${CMAKE_SOURCE_DIR}/docs)
  find_package(LATEX COMPONENTS PDFLATEX)
  if(LATEX_FOUND)
    add_custom_target(pdf make WORKING_DIRECTORY latex
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/latex/refman.pdf ${CMAKE_BINARY_DIR}/tclmpi_docs.pdf
      DEPENDS docs)
  endif()
  if(GIT_FOUND)
    add_custom_target(publish ${GIT_EXECUTABLE} branch --show-current > ${CMAKE_BINARY_DIR}/.current-branch
      COMMAND ${GIT_EXECUTABLE} checkout github-docs
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/html ${CMAKE_SOURCE_DIR}/docs
      COMMAND ${GIT_EXECUTABLE} add ${CMAKE_SOURCE_DIR}/docs
      COMMAND ${GIT_EXECUTABLE} commit -m 'update docs'
      COMMAND ${GIT_EXECUTABLE} checkout `cat ${CMAKE_BINARY_DIR}/.current-branch`
      DEPENDS docs)
  endif()
endif()

# installation
include(GNUInstallDirs)
install(TARGETS tclmpish DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_BINARY_DIR}/tclmpi.tcl ${CMAKE_BINARY_DIR}/pkgIndex.tcl
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/tcl${TCL_VERSION}/tclmpi${CMAKE_PROJECT_VERSION})
install(TARGETS _tclmpi
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/tcl${TCL_VERSION}/tclmpi${CMAKE_PROJECT_VERSION})
if(DOXYGEN_FOUND)
  install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target docs)")
endif()
install(DIRECTORY ${CMAKE_BINARY_DIR}/html examples
  DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(FILES ${CMAKE_BINARY_DIR}/latex/refman.pdf
  DESTINATION ${CMAKE_INSTALL_DOCDIR})

# packaging
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${TclMPI_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${TclMPI_VERSION_MINOR}")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES \\.git/ "build.*/" ".*~$" ".*/.*~$")
set(CPACK_VERBATIM_VARIABLES YES)
include(CPack)

# code checking with clang-tidy if available
find_program(CLANG_TIDY clang-tidy DOC "clang-tidy executable")
if("${CLANG_TIDY}" STREQUAL "CLANG_TIDY-NOTFOUND")
  add_custom_target(tidy COMMAND ${CMAKE_COMMAND} -E echo "clang-tidy not found")
else()
  get_target_property(MPI_INCLUDE_PATH MPI::MPI_C INTERFACE_INCLUDE_DIRECTORIES)
  add_custom_target(tidy
    COMMAND ${CLANG_TIDY} ${CMAKE_SOURCE_DIR}/_tclmpi.c
    -checks=-*,clang-analyzer-*,-clang-analyzer-cplusplus*,mpi-*,portability-*
    -- -I${MPI_INCLUDE_PATH} -I${TCL_INCLUDE_PATH} "-DPACKAGE_NAME=\\\"_tclmpi\\\"" "-DPACKAGE_VERSION=\\\"${CMAKE_PROJECT_VERSION}\\\"" -DBUILD_TCLMPISH -std=c99)
endif()

# configure tests
include(CTest)
enable_testing()
add_test(NAME RunShell01
  COMMAND $<TARGET_FILE:tclmpish> ${CMAKE_SOURCE_DIR}/tests/test_01.tcl
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(RunShell01 PROPERTIES
  PASS_REGULAR_EXPRESSION ".*test section 01.*total fail: 000.*")
add_test(NAME RunShell02
  COMMAND $<TARGET_FILE:tclmpish> ${CMAKE_SOURCE_DIR}/tests/test_02.tcl
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(RunShell02 PROPERTIES
  PASS_REGULAR_EXPRESSION ".*test section 02.*total fail: 000.*")
add_test(NAME RunShell03
  COMMAND mpirun -np 2 $<TARGET_FILE:tclmpish> ${CMAKE_SOURCE_DIR}/tests/test_03.tcl
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(RunShell03 PROPERTIES
  PASS_REGULAR_EXPRESSION ".*test section 03.*total fail: 000.*")
add_test(NAME RunShell04
  COMMAND mpirun -np 2 $<TARGET_FILE:tclmpish> ${CMAKE_SOURCE_DIR}/tests/test_04.tcl
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(RunShell04 PROPERTIES
  PASS_REGULAR_EXPRESSION ".*test section 04.*total fail: 000.*")
if(NOT ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin"))
  add_test(NAME RunPackage01
    COMMAND ${CMAKE_SOURCE_DIR}/tests/test_01.tcl
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  set_tests_properties(RunPackage01 PROPERTIES
    PASS_REGULAR_EXPRESSION ".*test section 01.*total fail: 000.*")
  add_test(NAME RunPackage02
    COMMAND ${CMAKE_SOURCE_DIR}/tests/test_02.tcl
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  set_tests_properties(RunPackage02 PROPERTIES
    PASS_REGULAR_EXPRESSION ".*test section 02.*total fail: 000.*")
  add_test(NAME RunPackage03
    COMMAND mpirun -np 2 ${CMAKE_SOURCE_DIR}/tests/test_03.tcl
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  set_tests_properties(RunPackage03 PROPERTIES
    PASS_REGULAR_EXPRESSION ".*test section 03.*total fail: 000.*")
  add_test(NAME RunPackage04
    COMMAND mpirun -np 2 ${CMAKE_SOURCE_DIR}/tests/test_04.tcl
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  set_tests_properties(RunPackage04 PROPERTIES
    PASS_REGULAR_EXPRESSION ".*test section 04.*total fail: 000.*")
endif()
