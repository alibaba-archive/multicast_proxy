#!/bin/sh

RPM_PATH=$PWD/rpmbuild_multicast
TMCC_PATH=$PWD
TMCC_VERSION=1.1
RELEASE=1

if [ $# -eq 1 ]; then
    TMCC_VERSION=$1
elif [ $# -eq 3 ]; then
    TMCC_VERSION=$1
    RPM_PATH=$2/rpmbuild_multicast
    TMCC_PATH=$3
fi

#mkdir -p $RPM_PATH/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
mkdir -p $RPM_PATH/BUILD
mkdir -p $RPM_PATH/BUILDROOT
mkdir -p $RPM_PATH/RPMS
mkdir -p $RPM_PATH/SOURCES
mkdir -p $RPM_PATH/SPECS
mkdir -p $RPM_PATH/SRPMS
echo "%_topdir $RPM_PATH" >~/.rpmmacros

cp -rf multi_server multi_server-$TMCC_VERSION

if [ -f multi_server-$TMCC_VERSION.tar.bz2 ];then
    rm -f multi_server-$TMCC_VERSION.tar.bz2
fi

tar -jcvf multi_server-$TMCC_VERSION.tar.bz2 multi_server-$TMCC_VERSION

cp -f multi_server-$TMCC_VERSION.tar.bz2 $RPM_PATH/SOURCES
cp -f multi_server.spec $RPM_PATH/SPECS
    
rm -f multi_server-$TMCC_VERSION.tar.bz2
rm -rf multi_server-$TMCC_VERSION

tmp_pwd=$TMCC_PATH
cd $RPM_PATH/SPECS

rpmbuild  --bb multi_server.spec  --define "name multi_server" --define "version $TMCC_VERSION" --define "release $RELEASE"

cp -f $RPM_PATH/RPMS/x86_64/multi_server-$TMCC_VERSION-$RELEASE.x86_64.rpm $tmp_pwd

rm -rf $RPM_PATH
