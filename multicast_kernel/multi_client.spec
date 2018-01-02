BuildRoot:  %{_tmppath}/%{name}-%{version}-%{release}-root
#Header 

Summary:    tmp multi client rpm package for VPC project 
Name:       %{name} 
Version:    %{version} 
Release:    %{release}
Vendor:     XXXX
Source:     %{name}-%{version}.tar.bz2
License:    GPL
URL:        Alibaba
Packager:   Alibaba
Group:      Applications/Internet
%define debug_package %{nil}
%description
tmp multi client module

%prep
#unzip source code package

%setup -q -b 0

%build
export MULTIC_VERSION=%{version}

#make
echo %{buildroot}
make clean
make

%install
mkdir -p %{buildroot}/usr/local/sbin
mkdir -p %{buildroot}/usr/local/src/multic/kmod
mkdir -p %{buildroot}/etc/init.d

cp -f ./tools/multi_client_control/multic_admin %{buildroot}/usr/local/sbin/
cp -f ./tools/multi_config_control/multic_config_admin %{buildroot}/usr/local/sbin/
cp -f ./kmod/multi_client.ko %{buildroot}/usr/local/src/multic/kmod/
cp -f ./scripts/multic %{buildroot}/etc/init.d/

%clean
rm -rf $RPM_BUILD_ROOT

%pre

%files
#%defattr(-,root,root)
/usr/local/sbin/*
/usr/local/src/multic/kmod/*
/etc/init.d/multic                                                                          
