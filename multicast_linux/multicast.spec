Name:       %{name}
Version:    %{version}
Release:    %{release}%{?dist}
Summary:    rpm pakcage for vnet_xgw

Group:      Alibaba/Application
License:    Proprietary
Source0:    %{name}-%{version}.tar.gz
BuildRoot:  %{_tmppath}/%{name}-%{version}-%{release}-root
Packager:   Alibaba/Aliyun

AutoReqProv: no

%description
rpm pakcage for vnet_xgw

%prep
%setup -q 


%build
make clean
make

%install
mkdir -p %{buildroot}/etc/multicast
mkdir -p %{buildroot}/usr/local/bin
mkdir -p %{buildroot}/usr/local/lib

cp -rf server_proxy/multis_admin %{buildroot}/usr/local/bin/
cp -rf server_tools/server_reload_list %{buildroot}/usr/local/bin/

mkdir -p %{buildroot}/etc/init.d/
chmod 755 ./script/multis_monitord
cp -rf ./script/multis_monitord %{buildroot}/usr/local/bin/
cp -rf ./script/multis_monitord %{buildroot}/etc/init.d/

chmod 755 ./script/multis_monitor
cp -rf ./script/multis_monitor %{buildroot}/usr/local/bin

cp -rf conf/* %{buildroot}/etc/multicast

#json
cp -rf json-c/.libs/libjson-c.so.3.0.0 %{buildroot}/usr/local/lib/
cp -rf json-c/.libs/libjson-c.a %{buildroot}/usr/local/lib/
cp -rf json-c/.libs/libjson-c.lai %{buildroot}/usr/local/lib/libjson-c.la
#pcap
cp -rf libpcap-1.7.4/libpcap.so.1.7.4 %{buildroot}/usr/local/lib/libpcap.so
cp -rf libpcap-1.7.4/libpcap.a %{buildroot}/usr/local/lib/

%clean
rm -rf $RPM_BUILD_ROOT

%files
/usr/local/bin/multis_monitor
/usr/local/bin/multis_admin
/usr/local/bin/server_reload_list
/usr/local/bin/multis_monitord
/etc/init.d/multis_monitord
%config(noreplace) /etc/multicast/*
/usr/local/lib/lib*

%post
ln -s -f /usr/local/lib/libjson-c.so.3.0.0 /usr/local/lib/libjson-c.so.3
ln -s -f /usr/local/lib/libjson-c.so.3.0.0 /usr/local/lib/libjson-c.so

if ! cat /etc/ld.so.conf | grep "/usr/local/lib" | grep -v grep > /dev/null; then
    echo "include /usr/local/lib" >> /etc/ld.so.conf
    ldconfig
fi

if ! cat ~/.bashrc | grep "export LD_LIBRARY_PATH=/usr/local/lib" | grep -v grep > /dev/null; then
    echo "export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH" >> ~/.bashrc
    source ~/.bashrc
fi
