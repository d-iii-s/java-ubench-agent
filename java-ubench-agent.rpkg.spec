Name: {{{ git_name name="java-ubench-agent" }}}
Version: {{{ git_version }}}
Release: 1%{?dist}
Summary: Multiplatform Java agent for hight-precision microbenchmarking.

License: ASL 2.0
URL: https://github.com/D-iii-S/java-ubench-agent/
VCS: {{{ git_vcs }}}

Source: {{{ git_pack }}}

Requires: papi
BuildRequires: papi-devel
BuildRequires: java-1.8.0-openjdk-headless
BuildRequires: java-1.8.0-openjdk-devel
BuildRequires: ant

%description
Multiplatform Java agent for hight-precision microbenchmarking.


%global debug_package %{nil}

%prep
{{{ git_setup_macro }}}

%build
export JAVA_HOME=/usr/lib/jvm/java
ant lib

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/opt/java-ubench-agent/
cp out/lib/libubench-agent.so $RPM_BUILD_ROOT/opt/java-ubench-agent/
cp out/lib/ubench-agent.jar $RPM_BUILD_ROOT/opt/java-ubench-agent/

%files
/opt/java-ubench-agent/libubench-agent.so
/opt/java-ubench-agent/ubench-agent.jar

%changelog
{{{ git_changelog }}}

