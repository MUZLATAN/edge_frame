#!/usr/bin/env bash

set -xeuo pipefail

VERSION=$1

# compile

mkdir -p _build
cd _build && rm -rf * && cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/$2.cmake -DPLATFORM=$2 -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)
cd ..

# clean
#find deploy/files/home/nvidia/FLOW/bin/ -type d -print | grep / | xargs rm -rf
rm -f deploy/files/home/nvidia/FLOW/config/*
rm -f deploy/*.tar
rm -f deploy/*.tar.gz

#create update version package
mkdir -p deploy/files/home/nvidia/FLOW/bin/
mkdir -p deploy/files/home/nvidia/FLOW/bin/${VERSION}/
mkdir -p deploy/files/home/nvidia/FLOW/models/
mkdir -p deploy/files/home/nvidia/FLOW/lib/
mkdir -p deploy/files/home/nvidia/FLOW/config/

# move
mv _build/edge deploy/files/home/nvidia/FLOW/bin/${VERSION}/algo-${VERSION}
cp $(find _build/ -name "*.so") deploy/files/home/nvidia/FLOW/lib/
rm -rf _build

cp config/service_default.json deploy/files/home/nvidia/FLOW/config/

# NOTE: this is for FileTrans
wget https://whale-cv-edge.oss-cn-shanghai.aliyuncs.com/FileTrans-38f384755a15d0947d37179c844b3891.tar.gz
tar xvf FileTrans-38f384755a15d0947d37179c844b3891.tar.gz
rm FileTrans-38f384755a15d0947d37179c844b3891.tar.gz
mv FileTrans deploy/files/home/nvidia/FLOW/bin/FileTrans

cp scripts/check deploy/files/home/nvidia/FLOW/bin/
cp scripts/service deploy/files/home/nvidia/FLOW/bin/

cd deploy

# make tar
tar czf algo-${VERSION}.tar.gz *

# rename tar
MD5=$(md5sum algo-${VERSION}.tar.gz | cut -d' ' -f1)
echo "MD5:" $MD5
mv algo-${VERSION}.tar.gz algo-${VERSION}-${MD5}.tar.gz

cd ..
python3 scripts/transFileToOss.py --filename ./deploy/algo-${VERSION}-${MD5}.tar.gz
