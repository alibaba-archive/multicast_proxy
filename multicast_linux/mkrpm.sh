#!/bin/bash

version="1.0.3"
release=1
name="multicast_usr_tool"

WORK_DIR=`pwd`
PROG=multicast

if [ "$WORK_DIR" == "" ] ; then
    echo "please set WORK_DIR env."
    exit 1
fi

if [ ! -f ./mkrpm.sh ]; then
    echo "please run this script in directory where mkrpm.sh located in"
    exit 1
fi

rm -rf *.tar.gz

if [ -d ./rpmbuild ] ; then
    tar -czf ${PROG}.tar.gz * --exclude rpmbuild
else
    tar -czf ${PROG}.tar.gz *
fi

# create necessary directories
[ -d ./rpmbuild/SOURCES/${PROG} ] && rm -rf ./rpmbuild/SOURCES/${PROG}
mkdir -p ./rpmbuild/SOURCES/${PROG}

build_od_arpproxy()
{
    # cp source code
    cd ${WORK_DIR}
    cp -rf ${PROG}.tar.gz rpmbuild/SOURCES/$PROG
    cd ${WORK_DIR}/rpmbuild/SOURCES/${PROG}
    tar -xzf ${PROG}.tar.gz
    rm -rf ${PROG}.tar.gz
    cd ${WORK_DIR}/rpmbuild/SOURCES/
	mv $PROG $name-$version

    # archive source
    rm -rf $name-$version.tar.gz
   	tar -czf $name-$version.tar.gz $name-$version
	rm -rf $name-$version

    # rpmbuild
    cd ${WORK_DIR}
    rpmbuild --define "_topdir ${WORK_DIR}/rpmbuild" \
    --define "name $name" \
    --define "version $version" \
    --define "release $release" \
    -bb ${WORK_DIR}/${PROG}.spec
}

build_od_arpproxy

