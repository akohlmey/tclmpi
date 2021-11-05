%{!?tcl_version: %global tcl_version %(echo 'puts $tcl_version' | tclsh)}
%{!?tcl_sitearch: %global tcl_sitearch %{_libdir}/tcl%{tcl_version}}
%{!?tcl_sitelib: %global tcl_sitelib %{_datadir}/tcl%{tcl_version}}
%global with_mpich 1
Name:          tclmpi
Version:       1.2
Release:       9
Summary:       Tcl bindings of the Message Passing Interface (MPI)

License:       BSD
URL:           git@github.com:akohlmey/tclmpi
Source0:       %{name}-%{version}.tar.gz

BuildRequires: tcl-devel >= 8.6 gcc cmake make binutils
Requires:      tcl >= 8.6
%if 0%{?rhel}
Requires:      environment-modules
%endif
%if 0%{?fedora}%{?rhel}
Requires:      environment(modules)
%if !0%{?el7}
BuildRequires: rpm-mpi-hooks
%endif
%endif

%description
TclMPI provides MPI support for Tcl scripts. It supports a commonly
used subset of the the MPI-1/MPI-2 specification with an interface
following the MPI C language bindings but with some adjustments for
Tcl dynamic typing. It supports point-to-point (sends, receives)
and collective (broadcasts, scatters, gathers) communications.

%package doc
BuildRequires: doxygen
Requires:      %{name} = %{version}-%{release}
Summary:       Documentation and examples for TclMPI

%description doc
This package contains HTML format documentation and example scripts
for TclMPI.

%package openmpi
%global mpi_family openmpi
BuildRequires: openmpi-devel
Requires:      %{name} = %{version}-%{release}
Summary:       Tcl bindings for MPI, Open MPI version
Provides:      tclmpi-runtime = %{version}-%{release}
Provides:      tclmpi-openmpi = %{version}-%{release}
%description openmpi
This package contains %{name} binary files compiled against Open MPI.

%if %{with_mpich}
%global mpi_family mpich
%package mpich
BuildRequires: mpich-devel
Requires:      %{name} = %{version}-%{release}
Summary:       Tcl bindings for MPI, MPICH version
Provides:      tclmpi-runtime = %{version}-%{release}
Provides:      tclmpi-mpich = %{version}-%{release}
%description mpich
This package contains %{name} binary files compiled against MPICH.
%endif

%prep
%autosetup

%build
%global mpi_family openmpi
%{_openmpi_load}
ompi_info
%cmake
%cmake_build
mv %_vpath_builddir openmpi
%{_openmpi_unload}

%if %{with_mpich}
%global mpi_family mpich
%{_mpich_load}
%cmake
%cmake_build
mv %_vpath_builddir mpich
%{_mpich_unload}
%endif

%install
%global mpi_family openmpi
%{_openmpi_load}
mv openmpi %_vpath_builddir
%cmake_install
mkdir -p %{buildroot}%{_libdir}/openmpi/bin
mv %{buildroot}%{_bindir}/tclmpish %{buildroot}%{_libdir}/openmpi/bin/tclmpish
mkdir -p %{buildroot}%{_libdir}/openmpi/lib/tcl%{tcl_version}/%{name}%{version}
mv %{buildroot}%{tcl_sitearch}/%{name}%{version}/_tclmpi.so %{buildroot}%{_libdir}/openmpi/lib/tcl%{tcl_version}/%{name}%{version}
mkdir -p %{buildroot}%{_libdir}/openmpi/share/man/man1
mv %{buildroot}%{_mandir}/man1/tclmpish.1 %{buildroot}%{_libdir}/openmpi/share/man/man1
mv %_vpath_builddir openmpi
%{_openmpi_unload}

%if %{with_mpich}
%global mpi_family mpich
%{_mpich_load}
mv mpich %_vpath_builddir
%cmake_install
mkdir -p %{buildroot}%{_libdir}/mpich/bin
mv %{buildroot}%{_bindir}/tclmpish %{buildroot}%{_libdir}/mpich/bin/tclmpish
mkdir -p %{buildroot}%{_libdir}/mpich/lib/tcl%{tcl_version}/%{name}%{version}
mv %{buildroot}%{tcl_sitearch}/%{name}%{version}/_tclmpi.so %{buildroot}%{_libdir}/mpich/lib/tcl%{tcl_version}/%{name}%{version}
mkdir -p %{buildroot}%{_libdir}/mpich/share/man/man1
mv %{buildroot}%{_mandir}/man1/tclmpish.1 %{buildroot}%{_libdir}/mpich/share/man/man1
mv %_vpath_builddir mpich
%{_mpich_unload}
%endif

mv %{buildroot}%{_docdir}/TclMPI %{buildroot}%{_docdir}/tclmpi

%files
%defattr(-,root,root)
%license LICENSE
%doc README.md CITATION.cff DESCRIPTION.txt
%{tcl_sitearch}/%{name}%{version}/pkgIndex.tcl
%{tcl_sitearch}/%{name}%{version}/tclmpi.tcl

%files doc
%attr(0755,root,root) %{_docdir}/tclmpi/examples/*.tcl
%doc %{_docdir}/tclmpi/html

%files openmpi
%global mpi_family openmpi
%{_libdir}/openmpi/bin
%{_libdir}/openmpi/lib/tcl%{tcl_version}/%{name}%{version}
%{_libdir}/openmpi/share/man/man1/tclmpish.1

%if %{with_mpich}
%global mpi_family mpich
%files mpich
%{_libdir}/mpich/bin
%{_libdir}/mpich/lib/tcl%{tcl_version}/%{name}%{version}
%{_libdir}/mpich/share/man/man1/tclmpish.1
%endif

%changelog
- Mention launchpad PPA for Ubuntu (akohlmey@gmail.com)
- update debian packaging files (akohlmey@gmail.com)

* Fri Nov 05 2021 Axel Kohlmeyer <akohlmey@gmail.com> 1.2-9
- fix multiple manpage installation issues

* Fri Nov 05 2021 Axel Kohlmeyer <akohlmey@gmail.com> 1.2-7
- add files for packaging for a launchpad ppa
- Add manpage for tclmpish

* Fri Nov 05 2021 Axel Kohlmeyer <akohlmey@gmail.com>
- add files for packaging for a launchpad ppa
- Add manpage for tclmpish

* Wed Nov 03 2021 Axel Kohlmeyer <akohlmey@gmail.com> 1.2-6
- Improve building html docs on systems without latex. include logo image
- Documentation updates and corrections

* Tue Nov 02 2021 Axel Kohlmeyer <akohlmey@gmail.com> 1.2-5
- recover building rpm packages on copr

* Tue Nov 02 2021 Axel Kohlmeyer <akohlmey@gmail.com> 1.2-4
- add support for creating an NSIS installer on Windows
- add icon and logo image for installer
- mention download of windows binaries on homepage
- change license text to read better in Windows installer
- remove obsolte backward compatibility in unit tests
- enable manual workflow dispatch feature

* Sun Oct 10 2021 Axel Kohlmeyer <akohlmey@gmail.com> 1.2-3
- must not delete variable with name of shared object

* Sun Oct 10 2021 Axel Kohlmeyer <akohlmey@gmail.com> 1.2-2
- exclude files and folders required for tito from exported source tar packages
- add support for a copr hosted Fedora Linux repository

* Sun Oct 10 2021 Axel Kohlmeyer <akohlmey@gmail.com> 1.2-1
- step version to 1.2

* Sun Oct 10 2021 Axel Kohlmeyer <akohlmey@gmail.com> 1.1.1-1
- Initial build of TclMPI with tito for copr
