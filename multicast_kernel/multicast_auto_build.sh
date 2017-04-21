#!/bin/sh
#****************************************************************#
# ScriptName: multicast_auto_build.sh
# Author: bangjie.jbj@alibaba-inc.com
# Create Date: 2014-11-30 15:19
# Function: 
#***************************************************************#

WORK_DIR=${WS_BASE}
VERSION=${BUILD_NUMBER}

if [ "$WORK_DIR" == "" ] || [ "$VERSION" == "" ] ; then
    echo "please set WS_BASE and BUILD_NUMBER env."
    exit 1
fi

mkdir -p ${WORK_DIR}/rpm_root
mkdir -p ${WORK_DIR}/rpm_release

cd ${WORK_DIR}
rpm_root=${WORK_DIR}/rpm_root
multicast_dir=${WORK_DIR}

#build multicast client
bash ${WORK_DIR}/multicast/multicast/tmcc_client_auto_rpm.sh "${VERSION}" "${rpm_root}" "${multicast_dir}"
ret=$?

rpm_num=`ls -l ${rpm_root}/rpmbuild_multicast/RPMS/x86_64/*.rpm | wc -l`

if [ "$ret" == "0" ] && (($rpm_num>0)); then
    cp ${rpm_root}/rpmbuild_multicast/RPMS/x86_64/*.rpm ${WORK_DIR}/rpm_release/ 
    rm -rf ${rpm_root}/rpmbuild_multicast
else
    cd ${WORK_DIR}
    rm -rf ${rpm_root}/rpmbuild_multicast
    exit 1
fi

#build multicast server
bash ${WORK_DIR}/multicast/multicast/tmcc_server_auto_rpm.sh "${VERSION}" "${rpm_root}" "${multicast_dir}"
ret=$?

rpm_num=`ls -l ${rpm_root}/rpmbuild_multicast/RPMS/x86_64/*.rpm | wc -l`

if [ "$ret" == "0" ] && (($rpm_num>0)); then
    cp ${rpm_root}/rpmbuild_multicast/RPMS/x86_64/*.rpm ${WORK_DIR}/rpm_release/ 
    rm -rf ${rpm_root}/rpmbuild_multicast
else
    cd ${WORK_DIR}
    rm -rf ${rpm_root}/rpmbuild_multicast
    exit 1
fi

cd ${WORK_DIR}
exit 0
