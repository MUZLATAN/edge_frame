#!/usr/bin/env bash
set -xeuo pipefail

VERSION=$1
PLATFORM=$2

EXEC_PATH='files/mnt/whale/service/algo'

if [[ "${PLATFORM}" == "arm" ]]; then
    EXEC_PATH='files/home/nvidia/FLOW'
elif [[ "${PLATFORM}" == "hisi" ]]; then
    echo ""
else
    echo "ERROR: ${PLATFORM} is not support!!!"
    exit 1
fi

echo "VERSION: ${VERSION}, PLATFORM: ${PLATFORM}, EXEC_PATH: ${EXEC_PATH}"

function cleanPack() {
    rm -rf ${EXEC_PATH}/bin/algo* ${EXEC_PATH}/lib ${EXEC_PATH}/models ${EXEC_PATH}/logs
    if [[ "${PLATFORM}" == "arm" ]]; then
        rm -rf ${EXEC_PATH}/bin/FileTrans
    elif [[ "${PLATFORM}" == "hisi" ]]; then
        rm -rf files/mnt/whale/daemon/conf/whale-daemon
    fi
}

set +e
rm package/*.tar.*
set -e

cd package/${PLATFORM}

# cleanPack
mkdir -p ${EXEC_PATH}/bin/
mkdir -p ${EXEC_PATH}/lib/
mkdir -p ${EXEC_PATH}/logs/

# lib && models
if [[ "${PLATFORM}" == "arm" ]]; then
    wget https://whale-cv-edge.oss-cn-shanghai.aliyuncs.com/FileTrans-38f384755a15d0947d37179c844b3891.tar.gz
    tar xvf FileTrans-38f384755a15d0947d37179c844b3891.tar.gz
    rm FileTrans-38f384755a15d0947d37179c844b3891.tar.gz
    mv FileTrans ${EXEC_PATH}/bin/FileTrans
elif [[ "${PLATFORM}" == "hisi" ]]; then
    wget https://whale-cv-video.oss-cn-hangzhou.aliyuncs.com/hisi_pack.tar.gz
    tar xvf hisi_pack.tar.gz
    rm hisi_pack.tar.gz
    cp -r mnt files/
    rm -rf mnt
fi
cd ../../

# compile
sed -i "s/#define ALGO_VERSION \".*\"/#define ALGO_VERSION \"${VERSION}\"/g" src/init.cpp
mkdir -p _build
cd _build && cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/${PLATFORM}.cmake -DPLATFORM=${PLATFORM} -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)
cd ../package
cp ../_build/edge ${PLATFORM}/${EXEC_PATH}/bin/algo-${VERSION}
cp $(find ../_build/ -name "*.so") ${PLATFORM}/${EXEC_PATH}/lib/
rm -rf ../_build

sed -i "s/VERSION="\'".*"\'"/VERSION="\'"${VERSION}"\'"/g" ${PLATFORM}/upgrade.sh

cd ${PLATFORM}
# update version value
if [[ "${PLATFORM}" == "hisi" ]]; then
    sed -i "s/version=.*/version=${VERSION}/g" files/mnt/whale/daemon/conf/algo.conf
fi
tar czf algo-${VERSION}.tar.gz files upgrade.sh
mv algo-${VERSION}.tar.gz ../
# clean
cleanPack
cd ..

MD5=$(md5sum algo-${VERSION}.tar.gz | cut -d' ' -f1)
echo "MD5:" $MD5
mv algo-${VERSION}.tar.gz algo-${VERSION}-${MD5}.tar.gz

exit 0
