Name: cxxtest
Summary: CxxTest Testing Framework for C++
Version: 1.10.2
Release: 1
Copyright: LGPL
Group: Development/C++
Source: cxxtest-%{version}.tgz
BuildRoot: /tmp/cxxtest-build
BuildArch: noarch
Prefix: /usr

%description
CxxTest is a JUnit/CppUnit/xUnit-like framework for C++.
Its advantages over existing alternatives are that it:
 - Doesn't require RTTI
 - Doesn't require member template functions
 - Doesn't require exception handling
 - Doesn't require any external libraries (including memory management, 
   file/console I/O, graphics libraries)

%prep
%setup -n cxxtest

%build

%install
install -m 755 -o 0 -g 0 -d $RPM_BUILD_ROOT/usr/include/cxxtest
install -m 644 -o 0 -g 0 cxxtest/* $RPM_BUILD_ROOT/usr/include/cxxtest/
install -m 755 -o 0 -g 0 -d $RPM_BUILD_ROOT/usr/bin
install -m 755 -o 0 -g 0 cxxtestgen.pl $RPM_BUILD_ROOT/usr/bin/

%clean
rm -rf $RPM_BUILD_ROOT

%files
%doc README
%doc sample
/usr/include/cxxtest
/usr/bin/cxxtestgen.pl

