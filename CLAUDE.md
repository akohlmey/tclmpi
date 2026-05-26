# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this project is

TclMPI wraps an MPI library as a Tcl package: a single C source file (`_tclmpi.c`) registers MPI wrappers as Tcl commands in the `tclmpi::` namespace, with Tcl-side glue and a few convenience procs in `tclmpi.tcl.in`. Requires Tcl ≥ 8.6 and MPI ≥ 2. Online docs: https://akohlmey.github.io/tclmpi/.

## Build, test, docs

CMake out-of-tree build. Two artifacts come out of the same C source:
- `_tclmpi.so` — loadable Tcl extension, built with the Tcl stubs library.
- `tclmpish` — extended Tcl shell with TclMPI statically embedded (built with `-DBUILD_TCLMPISH`). Stubs are incompatible with this shell and a `#error` enforces it.

```sh
# Configure + build
cmake -B build .
cmake --build build

# Run all unit tests
ctest --test-dir build -V

# Run a single test (regex matches the CTest name)
ctest --test-dir build -R RunShell03 -V

# Run a test script directly
build/tclmpish tests/test_01.tcl                    # serial
mpirun -np 2 build/tclmpish tests/test_03.tcl       # parallel

# Documentation (needs doxygen; pdf also needs pdflatex)
cmake --build build --target html
cmake --build build --target pdf

# Static analysis
cmake --build build --target tidy
```

CTest layout: `test_01`/`test_02` are serial, `test_03`/`test_04` need 2 MPI ranks. Each test file is run twice — once against the `tclmpish` shell (`RunShell0X`), once against the system `tclsh` loading the shared object (`RunPackage0X`). The `RunPackage*` tests are skipped on macOS and use the env var `CTEST_SHARED_OBJECT` to locate `_tclmpi.so`. See the `add_test` block in `CMakeLists.txt`.

Useful CMake options (see `INSTALL`): `BUILD_TCLMPI_SHELL`, `ENABLE_TCL_STUBS`, `BUILD_TESTING`, `DOWNLOAD_MPICH4WIN` (cross-compile to Windows).

## Architecture notes

- **One C file**: `_tclmpi.c` (~3100 lines) is the entire C side. The list of every Tcl command this package exposes lives in `tclmpi_init_api()` (around lines 2944–3005). Add new commands there.
- **Two binaries, one source**: `BUILD_TCLMPISH` switches the file between "loadable module" and "main()-bearing shell". The two modes are mutually exclusive with `USE_TCL_STUBS`.
- **String-labeled MPI handles**: MPI communicators and requests don't fit in Tcl values, so they're given string labels (`tclmpi::comm%d`, `tclmpi::req%d`) and tracked in two internal linked lists (`first_comm`, `first_req`). Translation helpers: `mpi2tcl_comm` / `tcl2mpi_comm` / `tclmpi_add_comm` / `tclmpi_del_comm` and `tclmpi_add_req` / `tclmpi_find_req` / `tclmpi_del_req`.
- **Template files**: `tclmpi.tcl.in`, `pkgIndex.tcl.in`, `tests/harness.tcl.in`, `tclmpi.nsis.in` are configured by CMake (`@PROJECT_VERSION@` etc.). Always run tests/examples from a build directory — the source-tree `.tcl.in` files are not loadable as-is.
- **Doxygen for Tcl**: Doxygen no longer parses Tcl, so docstrings in `.tcl.in` files are hidden behind a `#X#` prefix. `TclFileToDox()` in `CMakeLists.txt` strips the prefix into a generated `.dox` file. When documenting Tcl code, edit the `#X#` lines — don't invent a different comment style.
- **Convenience wrappers** like `tclmpi::waitall` are implemented in Tcl in `tclmpi.tcl.in`, not in C.

## Branch / environment notes

Branch `port-to-tcl9` is in progress. Two pre-configured build trees exist locally:
- `build/` — against system Tcl 8.6.
- `build-tcl9/` — against `/home/akohlmey/tcl9/include`.

When changing the C side, build and run `ctest` in both to verify Tcl 8.6 and Tcl 9 compatibility.

## Style

`.clang-format` at repo root governs C formatting. The `tidy` CMake target runs `clang-tidy` with `clang-analyzer-*`, `mpi-*`, and `portability-*` checks enabled.
