# CMake file for the TclMPI Tcl wrapper around MPI

cmake_minimum_required(VERSION 3.16)
project(TclMPI VERSION 1.1 DESCRIPTION "Tcl Bindings for MPI" LANGUAGES C)

# compilation settings and options
option(ENABLE_TCL_STUBS "Build TclMPI using the Tcl stub library" ON)
option(BUILD_TCLMPI_SHELL "Build Tcl interpreter with TclMPI embedded" ON)
mark_as_advanced(TTK_STUB_LIBRARY)

# tweaks
set(BUILD_SHARED_LIBS TRUE)
set(MPI_CXX_SKIP_MPICXX TRUE)

# find packages
find_package(MPI REQUIRED)
find_package(TCL)
if(!TCL_FOUND)
  message(FATAL_ERROR "Must have Tcl installed to build TclMPI")
endif()
if(ENABLE_TCL_STUBS)
  find_package(TclStub)
endif()
find_package(Doxygen OPTIONAL_COMPONENTS dot)

# extract Tcl version number from tcl.h
file(READ ${TCL_INCLUDE_PATH}/tcl.h _tcl_header)
string(REGEX MATCH "TCL_VERSION[ \t]*\"[0-9]\.[0-9]*\"" _ ${_tcl_header})
string(REGEX MATCH "[0-9]\.[0-9]*" TCL_VERSION ${_})

# build shared object
add_library(_tclmpi MODULE _tclmpi.c)
set_target_properties(_tclmpi PROPERTIES PREFIX "")
target_include_directories(_tclmpi PRIVATE ${TCL_INCLUDE_PATH})
target_compile_definitions(_tclmpi PRIVATE -DPACKAGE_NAME="_tclmpi" -DPACKAGE_VERSION="${CMAKE_PROJECT_VERSION}")
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

# embed version numbers
configure_file(tclmpi.tcl.in tclmpi.tcl @ONLY)
configure_file(pkgIndex.tcl.in pkgIndex.tcl @ONLY)
configure_file(tests/harness.tcl.in harness.tcl @ONLY)

# build documentation
if(DOXYGEN_FOUND)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md")
  set(DOXYGEN_GENERATE_LATEX TRUE)
  doxygen_add_docs(docs README.md _tclmpi.c ${CMAKE_BINARY_DIR}/tclmpi.tcl ${CMAKE_BINARY_DIR}/harness.tcl)
  set_property(TARGET docs APPEND PROPERTY ADDITIONAL_CLEAN_FILES latex)
  find_package(LATEX REQUIRED COMPONENTS PDFLATEX)
  if(LATEX_FOUND)
    add_custom_command(TARGET docs POST_BUILD COMMAND make WORKING_DIRECTORY latex)
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
