message(STATUS "Extracting documentation from Tcl files")

file(READ ${CMAKE_BINARY_DIR}/tclmpi.tcl TCLMPI_DOX)
#string(REGEX MATCHALL "#X#[^\\n]*\\n" TCLMPI_DOX ${TCLMPI_TCL})
#string(REGEX REPLACE "#X#([^\\n]*)\\n" "\\1\\n" TCLMPI_DOX ${_})
file(WRITE ${CMAKE_BINARY_DIR}/tclmpi.dox "${TCLMPI_DOX}")

file(READ ${CMAKE_BINARY_DIR}/harness.tcl HARNESS_DOX)
file(WRITE ${CMAKE_BINARY_DIR}/harness.dox "${HARNESS_DOX}")
